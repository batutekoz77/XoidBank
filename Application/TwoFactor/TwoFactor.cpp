#include "TwoFactor.hpp"
#include "../../Main/Config.hpp"
#include "../../Include/CookieUtils.hpp"
#include "../../Include/RateLimit/RateLimit.hpp"
#include <cctype>

namespace {
    bool IsValidCodeFormat(const std::string& code) {
        if (code.length() != 4) return false;
        for (char c : code) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        return true;
    }
}

void TwoFactorRouter::RegisterRoutes(CrowApp& app, UsersBase& usersBase, SessionManager& sessionManager, RateLimiter& rateLimiter) {
    CROW_ROUTE(app, "/2FA/Email").methods(crow::HTTPMethod::GET)([&sessionManager, &rateLimiter](const crow::request& req) {
        std::string clientIp = GetClientIp(req);

        if (!rateLimiter.CheckSimpleCooldown("email:" + clientIp, 2)) {}

        std::string pendingSessionId = GetCookieValue(req, "pending_session_id");
        auto email = sessionManager.GetEmailForPendingSession(pendingSessionId);

        if (!email.has_value()) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_AUTHENTICATION_SESSION;
            return crow::response(Status::UNAUTHORIZED, response);
        }

        crow::json::wvalue response;
        response["success"] = true;
        response["email"] = email.value();
        return crow::response(Status::SUCCESS, response);
    });

    CROW_ROUTE(app, "/2FA/Verify").methods(crow::HTTPMethod::POST)([&sessionManager, &rateLimiter](const crow::request& req) {
        std::string clientIp = GetClientIp(req);

        RateLimitResult limitResult = rateLimiter.CheckRequest(clientIp, 8, 300, 600);

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

        std::string pendingSessionId = GetCookieValue(req, "pending_session_id");
        auto body = crow::json::load(req.body);

        if (!body || !body.has("code")) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_REQUEST;
            return crow::response(Status::BAD_REQUEST, response);
        }

        std::string code = body["code"].s();

        if (!IsValidCodeFormat(code)) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_REQUEST;
            return crow::response(Status::BAD_REQUEST, response);
        }

        auto pendingData = sessionManager.ValidatePendingSession(pendingSessionId);

        if (!pendingData.has_value()) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_AUTHENTICATION_SESSION;
            return crow::response(Status::UNAUTHORIZED, response);
        }

        TwoFactorResult result;
        if (Config::TestMode && code == Config::Bypass2FA) { result = TwoFactorResult::Success; }
        else {
            result = sessionManager.VerifyTwoFactorCode(pendingSessionId, code);

            if (result == TwoFactorResult::WrongCode) {
                crow::json::wvalue response;
                response["success"] = false;
                response["message"] = Message::INVALID_TWO_FACTOR_CODE;
                return crow::response(Status::UNAUTHORIZED, response);
            }

            if (result == TwoFactorResult::Expired) {
                crow::json::wvalue response;
                response["success"] = false;
                response["message"] = Message::TWO_FACTOR_CODE_EXPIRED;
                return crow::response(Status::UNAUTHORIZED, response);
            }

            if (result == TwoFactorResult::TooManyAttempts) {
                crow::json::wvalue response;
                response["success"] = false;
                response["message"] = Message::TOO_MANY_TWO_FACTOR_ATTEMPTS;
                return crow::response(Status::TOO_MANY_REQUESTS, response);
            }

            if (result == TwoFactorResult::InvalidSession) {
                crow::json::wvalue response;
                response["success"] = false;
                response["message"] = Message::INVALID_AUTHENTICATION_SESSION;
                return crow::response(Status::UNAUTHORIZED, response);
            }
        }

        int userId = pendingData->UserId;
        bool rememberMe = pendingData->RememberMe;

        sessionManager.DestroyPendingSession(pendingSessionId);
        std::string sessionId = sessionManager.CreateSession(userId, rememberMe);

        if (sessionId.empty()) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INTERNAL_SERVER_ERROR;
            return crow::response(Status::INTERNAL_SERVER_ERROR, response);
        }

        rateLimiter.Reset(clientIp);

        crow::json::wvalue response;
        response["success"] = true;
        response["message"] = Message::TWO_FACTOR_VERIFICATION_SUCCESSFUL;

        crow::response res(Status::SUCCESS, response);

        res.add_header("Set-Cookie", "pending_session_id=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");

        int sessionMaxAge = rememberMe ? 2592000 : 86400;
        res.add_header("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=" + std::to_string(sessionMaxAge));

        if (rememberMe) {
            std::string rememberToken = sessionManager.CreateRememberToken(userId);
            if (!rememberToken.empty()) res.add_header("Set-Cookie", "remember_token=" + rememberToken + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=5184000");
        }

        return res;
    });
}