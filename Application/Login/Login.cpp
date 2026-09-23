#include "Login.hpp"
#include "../../Main/Config.hpp"
#include "../../Database/Users.hpp"
#include "../../Session/Session.hpp"
#include "../../Include/Mailer/Mailer.hpp"
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
}

void LoginRouter::RegisterRoutes(CrowApp& app, UsersBase& usersBase, SessionManager& sessionManager, RateLimiter& rateLimiter) {
    CROW_ROUTE(app, "/Login/Submit").methods(crow::HTTPMethod::POST)([&usersBase, &sessionManager, &rateLimiter](const crow::request& req) {

        std::string clientIp = GetClientIp(req);

        RateLimitResult limitResult = rateLimiter.CheckRequest(clientIp, 5, 300, 600);

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
            response["retryAfterSeconds"] = 600;
            return crow::response(Status::TOO_MANY_REQUESTS, response);
        }

        auto body = crow::json::load(req.body);

        if (!body || !body.has("email") || !body.has("password") || !body.has("rememberMe")) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_REQUEST;
            return crow::response(Status::BAD_REQUEST, response);
        }

        const auto rememberMeType = body["rememberMe"].t();
        if (rememberMeType != crow::json::type::True && rememberMeType != crow::json::type::False) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_REQUEST;
            return crow::response(Status::BAD_REQUEST, response);
        }

        const std::string email = Trim(body["email"].s());
        const std::string password = Trim(body["password"].s());
        const bool rememberMe = body["rememberMe"].b();

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

        int result = usersBase.validateLogin(email, password);

        if (result != static_cast<int>(LoginResult::Success)) {
            int statusCode = (result == static_cast<int>(LoginResult::AccountInactive)) ? Status::FORBIDDEN : Status::UNAUTHORIZED;
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = LoginResultMessages::ToString(result);
            return crow::response(statusCode, response);
        }

        auto userId = usersBase.getUserIdByEmail(email);

        if (!userId.has_value()) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INTERNAL_SERVER_ERROR;
            return crow::response(Status::INTERNAL_SERVER_ERROR, response);
        }

        auto pendingLogin = sessionManager.CreatePendingLogin(userId.value(), rememberMe);

        if (pendingLogin.PendingSessionId.empty() || pendingLogin.Code.empty()) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INTERNAL_SERVER_ERROR;
            return crow::response(Status::INTERNAL_SERVER_ERROR, response);
        }

        if (!Config::TestMode) {
            bool emailSent = Mailer::SendVerificationCode(email, pendingLogin.Code);
            if (!emailSent) {
                sessionManager.DestroyPendingSession(pendingLogin.PendingSessionId);
                crow::json::wvalue response;
                response["success"] = false;
                response["message"] = Message::UNABLE_TO_SEND_TWO_FACTOR_CODE;
                return crow::response(Status::INTERNAL_SERVER_ERROR, response);
            }
        }

        rateLimiter.Reset(clientIp);

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = Message::LOGIN_SUCCESSFUL;
        crow::response res(Status::SUCCESS, response);
        res.set_header("Set-Cookie", "pending_session_id=" + pendingLogin.PendingSessionId + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=120");

        return res;
    });
}