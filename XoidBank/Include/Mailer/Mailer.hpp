#pragma once
#include <string>

namespace Mailer {
    bool SendVerificationCode(const std::string& toEmail, const std::string& code);
}