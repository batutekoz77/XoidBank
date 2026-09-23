#pragma once
#include "../Include/Crow.hpp"
#include "../Database/Users.hpp"
#include "../Session/Session.hpp"
#include "../Include/RateLimit/RateLimit.hpp"
#include "../Include/RateLimit/GlobalRateLimitMiddleware.hpp"
#include "../Database/Accounts.hpp"
#include <string>

class Application {
public:
    Application();
    ~Application();

    void Run();

private:
    void RegisterRoutes();
    void RegisterStaticPageRoutes(const std::string& pageName, const std::string& pageFolderPath);
    void RegisterCustomPageRoutes();

    RateLimiter m_GlobalRateLimiter;
    crow::App<GlobalRateLimitMiddleware> m_App;

    UsersBase m_UsersBase;
    AccountsBase m_AccountsBase;
    SessionManager m_SessionManager;

    RateLimiter m_LoginRateLimiter;
    RateLimiter m_RegisterRateLimiter;
    RateLimiter m_TwoFactorRateLimiter;
    RateLimiter m_DashboardRateLimiter;
};