#pragma once
#include <sqlite_modern_cpp.h>
#include <string>
#include <optional>

struct PendingSessionData {
    int UserId;
    bool RememberMe;
};

enum class TwoFactorResult : int {
    Success = 0,
    WrongCode = 1,
    Expired = 2,
    TooManyAttempts = 3,
    InvalidSession = 4
};

class SessionManager {
public:
    struct PendingLoginResult {
        std::string PendingSessionId;
        std::string Code;
    };

    SessionManager(const std::string& dbPath);
    ~SessionManager();

    std::string CreateSession(int userId, bool rememberMe);
    std::optional<int> ValidateSession(const std::string& sessionId);
    void DestroySession(const std::string& sessionId);

    PendingLoginResult CreatePendingLogin(int userId, bool rememberMe);
    std::optional<PendingSessionData> ValidatePendingSession(const std::string& pendingSessionId);
    std::optional<std::string> GetEmailForPendingSession(const std::string& pendingSessionId);
    TwoFactorResult VerifyTwoFactorCode(const std::string& pendingSessionId, const std::string& code);
    void DestroyPendingSession(const std::string& pendingSessionId);

    std::string CreateRememberToken(int userId);
    std::optional<int> ValidateRememberToken(const std::string& token);
    void DestroyRememberToken(const std::string& token);

private:
    sqlite::database db;

    std::string GenerateToken();
    std::string GenerateFourDigitCode();

    static constexpr int MaxTwoFactorAttempts = 5;
    static constexpr int TwoFactorExpirySeconds = 120;
};