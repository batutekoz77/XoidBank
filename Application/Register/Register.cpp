#include "Register.hpp"
#include "../../Main/Config.hpp"
#include "../../Database/Users.hpp"
#include "../../Include/RateLimit/RateLimit.hpp"
#include <regex>


namespace {
    const std::regex EmailPattern(R"(^[^\s@]+@[^\s@]+\.[^\s@]+$)");

    std::string Trim(const std::string& value) {
        const auto start = value.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";

        const auto end = value.find_last_not_of(" \t\n\r");
        return value.substr(start, end - start + 1);
    }

    std::string RemoveWhitespace(const std::string& value) {
        std::string result;
        result.reserve(value.size());
        for (char c : value) { if (!std::isspace(static_cast<unsigned char>(c))) result += c; }
        return result;
    }

    bool IsValidBSN(const std::string& bsn) {
        if (Config::TestMode) return true;
        if (bsn.length() != 8 && bsn.length() != 9) return false;
        for (char c : bsn) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        std::string padded = bsn.length() == 8 ? "0" + bsn : bsn;
        int sum = 0;
        for (int i = 0; i < 8; ++i) {
            int digit = padded[i] - '0';
            sum += digit * (9 - i);
        }
        int lastDigit = padded[8] - '0';
        sum -= lastDigit;
        return (sum % 11) == 0;
    }
}

void RegisterRouter::RegisterRoutes(CrowApp& app, UsersBase& usersBase, RateLimiter& rateLimiter) {
    CROW_ROUTE(app, "/Register/Submit").methods(crow::HTTPMethod::POST)([&usersBase, &rateLimiter](const crow::request& req) {

        std::string clientIp = GetClientIp(req);

        RateLimitResult limitResult = rateLimiter.CheckRequest(clientIp, 3, 600, 1800);

        if (limitResult == RateLimitResult::OnCooldown) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::TOO_MANY_REQUESTS;
            response["retryAfterSeconds"] = rateLimiter.GetRemainingCooldown(clientIp);
            return crow::response(Status::TOO_MANY_REQUESTS, response);
        }

        if (limitResult == RateLimitResult::LimitExceeded) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::TOO_MANY_REQUESTS;
            response["retryAfterSeconds"] = 1800;
            return crow::response(Status::TOO_MANY_REQUESTS, response);
        }

        auto body = crow::json::load(req.body);

        if (!body || !body.has("fullName") || !body.has("email") || !body.has("password") || !body.has("confirmPassword") || !body.has("phoneNumber") || !body.has("BSN")) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_REQUEST;
            return crow::response(Status::BAD_REQUEST, response);
        }

        const std::string fullName = Trim(body["fullName"].s());
        const std::string email = Trim(body["email"].s());
        const std::string password = Trim(body["password"].s());
        const std::string confirmPassword = Trim(body["confirmPassword"].s());
        const std::string phoneNumber = RemoveWhitespace(Trim(body["phoneNumber"].s()));
        const std::string BSN = Trim(body["BSN"].s());

        if (fullName.empty() || fullName.length() > 100) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_REQUEST;
            return crow::response(Status::BAD_REQUEST, response);
        }

        if (email.empty() || email.length() > 254 || !std::regex_match(email, EmailPattern)) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::EMAIL_REQUIRED;
            return crow::response(Status::BAD_REQUEST, response);
        }

        if (password.empty() || password.length() > 128 || password.length() < 6) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::PASSWORD_REQUIRED;
            return crow::response(Status::BAD_REQUEST, response);
        }

        if (password != confirmPassword) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::PASSWORDS_DO_NOT_MATCH;
            return crow::response(Status::BAD_REQUEST, response);
        }

        if (phoneNumber.empty() || phoneNumber.length() > 20) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::PHONE_NUMBER_REQUIRED;
            return crow::response(Status::BAD_REQUEST, response);
        }

        if (BSN.empty()) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::BSN_REQUIRED;
            return crow::response(Status::BAD_REQUEST, response);
        }

        if (BSN.length() > 9 || !IsValidBSN(BSN)) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_BSN;
            return crow::response(Status::BAD_REQUEST, response);
        }

        int result = usersBase.addUser(fullName, email, password, phoneNumber, BSN);

        if (result != static_cast<int>(LoginResult::Success)) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = LoginResultMessages::ToString(result);

            bool isConflict = (result == static_cast<int>(LoginResult::EmailAlreadyExists)) || (result == static_cast<int>(LoginResult::PhoneAlreadyExists)) || (result == static_cast<int>(LoginResult::BSNAlreadyExists));
            int statusCode = isConflict ? Status::CONFLICT : Status::INTERNAL_SERVER_ERROR;
            return crow::response(statusCode, response);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = Message::REGISTER_SUCCESSFUL;
        return crow::response(Status::CREATED, response);
    });
}