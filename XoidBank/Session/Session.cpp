#include "Session.hpp"
#include <sodium.h>
#include <iostream>
#include <sstream>
#include <iomanip>

SessionManager::SessionManager(const std::string& dbPath) : db(dbPath) {
    try {
        db << "CREATE TABLE IF NOT EXISTS Sessions ("
            "SessionId TEXT PRIMARY KEY,"
            "UserId    INTEGER NOT NULL,"
            "CreatedAt TEXT NOT NULL DEFAULT (datetime('now')),"
            "ExpiresAt TEXT NOT NULL"
        ");";

        db << "CREATE TABLE IF NOT EXISTS PendingSessions ("
            "PendingSessionId TEXT PRIMARY KEY,"
            "UserId           INTEGER NOT NULL,"
            "RememberMe       INTEGER NOT NULL DEFAULT 0,"
            "Code             TEXT NOT NULL,"
            "AttemptCount     INTEGER NOT NULL DEFAULT 0,"
            "CreatedAt        TEXT NOT NULL DEFAULT (datetime('now')),"
            "ExpiresAt        TEXT NOT NULL"
        ");";

        db << "CREATE TABLE IF NOT EXISTS RememberTokens ("
            "Token     TEXT PRIMARY KEY,"
            "UserId    INTEGER NOT NULL,"
            "CreatedAt TEXT NOT NULL DEFAULT (datetime('now')),"
            "ExpiresAt TEXT NOT NULL"
        ");";
    }
    catch (const std::exception& e) { std::cerr << "Session DB initialization error: " << e.what() << std::endl; }
}

SessionManager::~SessionManager() {}

std::string SessionManager::GenerateToken() {
    unsigned char rawBytes[32];
    randombytes_buf(rawBytes, sizeof(rawBytes));

    std::ostringstream hexStream;
    for (unsigned char b : rawBytes) hexStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return hexStream.str();
}

std::string SessionManager::GenerateFourDigitCode() {
    uint32_t value = randombytes_uniform(10000);
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << value;
    return oss.str();
}

std::string SessionManager::CreateSession(int userId, bool rememberMe) {
    std::string sessionId = GenerateToken();
    int expiryDays = rememberMe ? 30 : 1;

    try {
        db << "INSERT INTO Sessions (SessionId, UserId, ExpiresAt) "
            "VALUES (?, ?, datetime('now', ?));"
            << sessionId << userId << ("+" + std::to_string(expiryDays) + " days");
    }
    catch (const std::exception& e) {
        std::cerr << "CreateSession error: " << e.what() << std::endl;
        return "";
    }
    return sessionId;
}

std::optional<int> SessionManager::ValidateSession(const std::string& sessionId) {
    if (sessionId.empty()) return std::nullopt;

    int userId = -1;
    bool found = false;

    try {
        db << "SELECT UserId FROM Sessions WHERE SessionId = ? AND ExpiresAt > datetime('now');"
            << sessionId
            >> [&](int id) { userId = id; found = true; };
    }
    catch (const std::exception& e) {
        std::cerr << "ValidateSession error: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (!found) return std::nullopt;
    return userId;
}

void SessionManager::DestroySession(const std::string& sessionId) {
    try { db << "DELETE FROM Sessions WHERE SessionId = ?;" << sessionId; }
    catch (const std::exception& e) { std::cerr << "DestroySession error: " << e.what() << std::endl; }
}

SessionManager::PendingLoginResult SessionManager::CreatePendingLogin(int userId, bool rememberMe) {
    std::string pendingId = GenerateToken();
    std::string code = GenerateFourDigitCode();

    try {
        db << "INSERT INTO PendingSessions (PendingSessionId, UserId, RememberMe, Code, AttemptCount, ExpiresAt) "
            "VALUES (?, ?, ?, ?, 0, datetime('now', ?));"
            << pendingId << userId << (rememberMe ? 1 : 0) << code
            << ("+" + std::to_string(TwoFactorExpirySeconds) + " seconds");
    }
    catch (const std::exception& e) {
        std::cerr << "CreatePendingLogin error: " << e.what() << std::endl;
        return { "", "" };
    }

    return { pendingId, code };
}

std::optional<PendingSessionData> SessionManager::ValidatePendingSession(const std::string& pendingSessionId) {
    if (pendingSessionId.empty()) return std::nullopt;

    PendingSessionData data{};
    bool found = false;

    try {
        db << "SELECT UserId, RememberMe FROM PendingSessions "
            "WHERE PendingSessionId = ? AND ExpiresAt > datetime('now');"
            << pendingSessionId
            >> [&](int userId, int rememberMe) {
            data.UserId = userId;
            data.RememberMe = (rememberMe != 0);
            found = true;
            };
    }
    catch (const std::exception& e) {
        std::cerr << "ValidatePendingSession error: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (!found) return std::nullopt;
    return data;
}

std::optional<std::string> SessionManager::GetEmailForPendingSession(const std::string& pendingSessionId) {
    if (pendingSessionId.empty()) return std::nullopt;

    std::string email;
    bool found = false;

    try {
        db << "SELECT Users.Email FROM PendingSessions "
            "JOIN Users ON Users.UserId = PendingSessions.UserId "
            "WHERE PendingSessions.PendingSessionId = ? AND PendingSessions.ExpiresAt > datetime('now');"
            << pendingSessionId
            >> [&](std::string e) { email = e; found = true; };
    }
    catch (const std::exception& e) {
        std::cerr << "GetEmailForPendingSession error: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (!found) return std::nullopt;
    return email;
}

TwoFactorResult SessionManager::VerifyTwoFactorCode(const std::string& pendingSessionId, const std::string& code) {
    if (pendingSessionId.empty()) return TwoFactorResult::InvalidSession;

    std::string storedCode;
    int attemptCount = 0;
    bool expired = false;
    bool found = false;

    try {
        db << "SELECT Code, AttemptCount, (ExpiresAt <= datetime('now')) FROM PendingSessions "
            "WHERE PendingSessionId = ?;"
            << pendingSessionId
            >> [&](std::string c, int attempts, int isExpired) {
            storedCode = c;
            attemptCount = attempts;
            expired = (isExpired != 0);
            found = true;
        };
    }
    catch (const std::exception& e) {
        std::cerr << "VerifyTwoFactorCode error: " << e.what() << std::endl;
        return TwoFactorResult::InvalidSession;
    }

    if (!found) return TwoFactorResult::InvalidSession;

    if (expired) {
        DestroyPendingSession(pendingSessionId);
        return TwoFactorResult::Expired;
    }

    if (attemptCount >= MaxTwoFactorAttempts) {
        DestroyPendingSession(pendingSessionId);
        return TwoFactorResult::TooManyAttempts;
    }

    if (code != storedCode) {
        try {
            db << "UPDATE PendingSessions SET AttemptCount = AttemptCount + 1 WHERE PendingSessionId = ?;"
                << pendingSessionId;
        }
        catch (const std::exception& e) {
            std::cerr << "VerifyTwoFactorCode attempt update error: " << e.what() << std::endl;
        }

        if (attemptCount + 1 >= MaxTwoFactorAttempts) {
            DestroyPendingSession(pendingSessionId);
            return TwoFactorResult::TooManyAttempts;
        }

        return TwoFactorResult::WrongCode;
    }

    return TwoFactorResult::Success;
}

void SessionManager::DestroyPendingSession(const std::string& pendingSessionId) {
    try {
        db << "DELETE FROM PendingSessions WHERE PendingSessionId = ?;" << pendingSessionId;
    }
    catch (const std::exception& e) {
        std::cerr << "DestroyPendingSession error: " << e.what() << std::endl;
    }
}

std::string SessionManager::CreateRememberToken(int userId) {
    std::string token = GenerateToken();

    try {
        db << "INSERT INTO RememberTokens (Token, UserId, ExpiresAt) "
            "VALUES (?, ?, datetime('now', '+60 days'));"
            << token << userId;
    }
    catch (const std::exception& e) {
        std::cerr << "CreateRememberToken error: " << e.what() << std::endl;
        return "";
    }
    return token;
}

std::optional<int> SessionManager::ValidateRememberToken(const std::string& token) {
    if (token.empty()) return std::nullopt;

    int userId = -1;
    bool found = false;

    try {
        db << "SELECT UserId FROM RememberTokens WHERE Token = ? AND ExpiresAt > datetime('now');"
            << token
            >> [&](int id) { userId = id; found = true; };
    }
    catch (const std::exception& e) {
        std::cerr << "ValidateRememberToken error: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (!found) return std::nullopt;
    return userId;
}

void SessionManager::DestroyRememberToken(const std::string& token) {
    try { db << "DELETE FROM RememberTokens WHERE Token = ?;" << token; }
    catch (const std::exception& e) { std::cerr << "DestroyRememberToken error: " << e.what() << std::endl; }
}