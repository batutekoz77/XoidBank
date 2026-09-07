#pragma once
#include "../../Include/Crow.hpp"
#include "../../Database/Users.hpp"
#include "../../Session/Session.hpp"
#include "../../Include/RateLimit/RateLimit.hpp"

class LoginRouter {
public:
    static void RegisterRoutes(CrowApp& app, UsersBase& usersBase, SessionManager& sessionManager, RateLimiter& rateLimiter);
};