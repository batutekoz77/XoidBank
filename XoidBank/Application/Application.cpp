#include "Application.hpp"
#include "../Main/Config.hpp"
#include "Login/Login.hpp"
#include "Register/Register.hpp"
#include "../Include/CookieUtils.hpp"
#include "TwoFactor/TwoFactor.hpp"
#include "../Include/Mailer/Mailer.hpp"

#include <sodium.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;


namespace {
    const std::string PagesRootPath = "Include/Pages";

    std::string ReadFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string GetContentType(const std::string& extension) {
        if (extension == ".html") return "text/html";
        if (extension == ".css")  return "text/css";
        if (extension == ".js")   return "application/javascript";
        return "text/plain";
    }

    bool IsPublicPage(const std::string& pageName) {
        return pageName == "Login" || pageName == "Register";
    }
}

Application::Application()
    : m_GlobalRateLimiter()
    , m_App(GlobalRateLimitMiddleware(m_GlobalRateLimiter, 200, 10, 300))
    , m_UsersBase(Config::DatabasePathUsers)
    , m_SessionManager(Config::DatabasePathUsers)
{
    if (sodium_init() < 0) { throw std::runtime_error("libsodium could not be initialized"); }
    RegisterRoutes();
}
Application::~Application() { }

void Application::RegisterStaticPageRoutes(const std::string& pageName, const std::string& pageFolderPath) {
    for (const auto& fileEntry : fs::directory_iterator(pageFolderPath)) {
        if (!fileEntry.is_regular_file()) continue;

        std::string filePath = fileEntry.path().string();
        std::string extension = fileEntry.path().extension().string();

        std::string routePath;
        bool isPageRoot = (extension == ".html");

        if (isPageRoot) routePath = "/" + pageName;
        else routePath = "/" + pageName + "/" + fileEntry.path().filename().string();

        std::string contentType = GetContentType(extension);
        bool isPublic = IsPublicPage(pageName);
        bool is2FA = (pageName == "2FA");

        if (isPageRoot && !isPublic) {
            m_App.route_dynamic(routePath)([this, filePath, contentType, is2FA](const crow::request& req) {
                if (is2FA) {
                    std::string pendingId = GetCookieValue(req, "pending_session_id");
                    if (!m_SessionManager.ValidatePendingSession(pendingId).has_value()) {
                        crow::response res;
                        res.code = 302;
                        res.set_header("Location", "/Login");
                        return res;
                    }
                }
                else {
                    std::string sessionId = GetCookieValue(req, "session_id");
                    if (!m_SessionManager.ValidateSession(sessionId).has_value()) {
                        crow::response res;
                        res.code = 302;
                        res.set_header("Location", "/Login");
                        return res;
                    }
                }

                crow::response res(ReadFile(filePath));
                res.set_header("Content-Type", contentType);
                return res;
            });
        }
        else {
            m_App.route_dynamic(routePath)([filePath, contentType]() {
                crow::response res(ReadFile(filePath));
                res.set_header("Content-Type", contentType);
                return res;
            });
        }
        std::cout << "[Application] Route eklendi: " << routePath << " -> " << filePath << std::endl;
    }
}

void Application::RegisterRoutes() {
    if (!fs::exists(PagesRootPath) || !fs::is_directory(PagesRootPath)) {
        std::cerr << "[Application] Couldn't find the Pages directory: " << PagesRootPath << std::endl;
        return;
    }

    m_App.route_dynamic("/")([this](const crow::request& req) {
        std::string sessionId = GetCookieValue(req, "session_id");
        if (m_SessionManager.ValidateSession(sessionId).has_value()) {
            crow::response res;
            res.code = 302;
            res.set_header("Location", "/Dashboard");
            return res;
        }

        std::string rememberToken = GetCookieValue(req, "remember_token");
        auto userId = m_SessionManager.ValidateRememberToken(rememberToken);

        if (userId.has_value()) {
            auto email = m_UsersBase.getEmailById(userId.value());

            if (email.has_value()) {
                auto pendingLogin = m_SessionManager.CreatePendingLogin(userId.value(), true);

                if (!pendingLogin.PendingSessionId.empty()) {
                    bool verificationReady = Config::TestMode || Mailer::SendVerificationCode(email.value(), pendingLogin.Code);

                    if (verificationReady) {
                        crow::response res;
                        res.code = 302;
                        res.set_header("Location", "/2FA");
                        res.set_header("Set-Cookie", "pending_session_id=" + pendingLogin.PendingSessionId + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=120");
                        return res;
                    }

                    m_SessionManager.DestroyPendingSession(pendingLogin.PendingSessionId);
                }
            }
        }

        crow::response res;
        res.code = 302;
        res.set_header("Location", "/Login");
        return res;
    });

    for (const auto& entry : fs::directory_iterator(PagesRootPath)) {
        if (!entry.is_directory()) continue;
        RegisterStaticPageRoutes(entry.path().filename().string(), entry.path().string());
    }

    RegisterCustomPageRoutes();
}

void Application::RegisterCustomPageRoutes() {
    LoginRouter::RegisterRoutes(m_App, m_UsersBase, m_SessionManager, m_LoginRateLimiter);
    RegisterRouter::RegisterRoutes(m_App, m_UsersBase, m_RegisterRateLimiter);
    TwoFactorRouter::RegisterRoutes(m_App, m_UsersBase, m_SessionManager, m_TwoFactorRateLimiter);
}

void Application::Run() {
    std::cout << "[Application] Starting the server: http://" << Config::Host << ":" << Config::Port << std::endl;
    m_App.bindaddr(Config::Host).port(Config::Port).multithreaded().run();
}