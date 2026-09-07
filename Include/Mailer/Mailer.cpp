#include "Mailer.hpp"
#include "../../Main/Config.hpp"
#include <curl/curl.h>
#include <sstream>
#include <cstring>
#include <iostream>

namespace {
    struct UploadStatus {
        const char* data;
        size_t bytesRead;
    };

    size_t PayloadSource(void* ptr, size_t size, size_t nmemb, void* userp) {
        UploadStatus* upload = static_cast<UploadStatus*>(userp);
        size_t remaining = std::strlen(upload->data + upload->bytesRead);
        size_t bufferSize = size * nmemb;

        if (remaining == 0) return 0;

        size_t toCopy = remaining < bufferSize ? remaining : bufferSize;
        std::memcpy(ptr, upload->data + upload->bytesRead, toCopy);
        upload->bytesRead += toCopy;

        return toCopy;
    }
}

bool Mailer::SendVerificationCode(const std::string& toEmail, const std::string& code) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[Mailer] curl_easy_init failed" << std::endl;
        return false;
    }

    std::ostringstream body;
    body << "To: " << toEmail << "\r\n"
        << "From: " << Config::SmtpFromAddress << "\r\n"
        << "Subject: Your XoidBank verification code\r\n"
        << "Content-Type: text/plain; charset=UTF-8\r\n"
        << "\r\n"
        << "Your XoidBank verification code is: " << code << "\r\n"
        << "This code will expire in 120 seconds.\r\n"
        << "If you did not request this, please ignore this email.\r\n";

    std::string payload = body.str();
    UploadStatus upload{ payload.c_str(), 0 };

    std::string mailFrom = "<" + Config::SmtpFromAddress + ">";
    std::string mailTo = "<" + toEmail + ">";

    curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
    curl_easy_setopt(curl, CURLOPT_USERNAME, Config::SmtpUsername.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, Config::SmtpPassword.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, mailFrom.c_str());

    struct curl_slist* recipients = nullptr;
    recipients = curl_slist_append(recipients, mailTo.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, PayloadSource);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

    CURLcode result = curl_easy_perform(curl);

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        std::cerr << "[Mailer] Failed to send email: " << curl_easy_strerror(result) << std::endl;
        return false;
    }

    return true;
}