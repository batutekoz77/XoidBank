#pragma once
#include "../../Include/Crow.hpp"
#include "../../Database/Users.hpp"
#include "../../Include/RateLimit/RateLimit.hpp"

class RegisterRouter {
public:
    static void RegisterRoutes(CrowApp& app, UsersBase& usersBase, RateLimiter& rateLimiter);
};