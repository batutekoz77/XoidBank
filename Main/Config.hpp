#pragma once
#include <string>
#include <filesystem>
#include <Windows.h>

namespace {
    inline std::string GetExecutableDirectory() {
        char buffer[MAX_PATH];
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        return std::filesystem::path(buffer).parent_path().string();
    }
}

namespace Config {
    inline const bool TestMode = false;
    inline const std::string Bypass2FA = "";

	inline const std::string Host = "127.0.0.1";
	inline const int Port = 8080;
    inline const std::string DatabasePathUsers = GetExecutableDirectory() + "\\Users.db";

    inline const std::string SmtpUsername = "youraddress@gmail.com";
    inline const std::string SmtpPassword = "gmail-app-password";
    inline const std::string SmtpFromAddress = "youraddress@gmail.com";
}

namespace Status {
    inline constexpr int SUCCESS = 200;
    inline constexpr int CREATED = 201;
    inline constexpr int ACCEPTED = 202;
    inline constexpr int NO_CONTENT = 204;


    inline constexpr int BAD_REQUEST = 400;
    inline constexpr int UNAUTHORIZED = 401;
    inline constexpr int FORBIDDEN = 403;
    inline constexpr int NOT_FOUND = 404;
    inline constexpr int METHOD_NOT_ALLOWED = 405;
    inline constexpr int CONFLICT = 409;
    inline constexpr int UNPROCESSABLE_ENTITY = 422;
    inline constexpr int TOO_MANY_REQUESTS = 429;


    inline constexpr int INTERNAL_SERVER_ERROR = 500;
    inline constexpr int NOT_IMPLEMENTED = 501;
    inline constexpr int BAD_GATEWAY = 502;
    inline constexpr int SERVICE_UNAVAILABLE = 503;
    inline constexpr int GATEWAY_TIMEOUT = 504;
}


namespace Message {
    inline constexpr const char* SUCCESS = "Success.";
    inline constexpr const char* INVALID_REQUEST = "Invalid request.";
    inline constexpr const char* NOT_FOUND = "Resource not found.";
    inline constexpr const char* METHOD_NOT_ALLOWED = "Method not allowed.";


    inline constexpr const char* LOGIN_SUCCESSFUL = "Login successful.";
    inline constexpr const char* REGISTER_SUCCESSFUL = "Registration successful.";
    inline constexpr const char* INVALID_CREDENTIALS = "Invalid email or password.";
    inline constexpr const char* ACCOUNT_DISABLED = "Account disabled.";
    inline constexpr const char* EMAIL_REQUIRED = "Email address is required.";
    inline constexpr const char* PASSWORD_REQUIRED = "Password is required.";
    inline constexpr const char* PASSWORDS_DO_NOT_MATCH = "Passwords do not match.";
    inline constexpr const char* PHONE_NUMBER_REQUIRED = "Phone number is required.";
    inline constexpr const char* BSN_REQUIRED = "BSN is required.";
    inline constexpr const char* INVALID_BSN = "Invalid BSN.";
    inline constexpr const char* EMAIL_ALREADY_EXISTS = "An account with this email already exists.";
    inline constexpr const char* BSN_ALREADY_EXISTS = "An account with this BSN already exists.";


    inline constexpr const char* TOO_MANY_REQUESTS = "Too many requests. Please try again later.";


    inline constexpr const char* TWO_FACTOR_VERIFICATION_SUCCESSFUL = "Verification successful.";
    inline constexpr const char* INVALID_TWO_FACTOR_CODE = "Invalid verification code.";
    inline constexpr const char* TWO_FACTOR_CODE_EXPIRED = "Verification code expired.";
    inline constexpr const char* TOO_MANY_TWO_FACTOR_ATTEMPTS = "Too many verification attempts.";
    inline constexpr const char* INVALID_AUTHENTICATION_SESSION = "Invalid authentication session.";
    inline constexpr const char* UNABLE_TO_SEND_TWO_FACTOR_CODE = "Unable to send verification code.";


    inline constexpr const char* INTERNAL_SERVER_ERROR = "Internal server error.";
    inline constexpr const char* SERVICE_UNAVAILABLE = "Service temporarily unavailable.";
}