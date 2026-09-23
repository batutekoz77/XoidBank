#pragma once
#include <crow.h>
#include "RateLimit.hpp"

struct GlobalRateLimitMiddleware {
    struct context {};

    RateLimiter& Limiter;
    int MaxRequests;
    int WindowSeconds;
    int CooldownSeconds;

    GlobalRateLimitMiddleware(RateLimiter& limiter, int maxRequests, int windowSeconds, int cooldownSeconds)
        : Limiter(limiter), MaxRequests(maxRequests), WindowSeconds(windowSeconds), CooldownSeconds(cooldownSeconds) {
    }

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        std::string clientIp = GetClientIp(req);

        RateLimitResult result = Limiter.CheckRequest(clientIp, MaxRequests, WindowSeconds, CooldownSeconds);

        if (result == RateLimitResult::OnCooldown || result == RateLimitResult::LimitExceeded) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = "Too many requests. Please slow down.";

            res.code = 429;
            res.set_header("Content-Type", "application/json");
            res.body = response.dump();
            res.end();
        }
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx) {}
};