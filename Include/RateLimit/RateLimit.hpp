#pragma once
#include <crow.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

enum class RateLimitResult : int {
    Allowed = 0,
    LimitExceeded = 1,
    OnCooldown = 2
};

class RateLimiter {
public:
    RateLimiter();
    RateLimitResult CheckRequest(const std::string& key, int maxAttempts, int windowSeconds, int cooldownSeconds);
    bool CheckSimpleCooldown(const std::string& key, int cooldownSeconds);
    int GetRemainingCooldown(const std::string& key);
    void Reset(const std::string& key);

private:
    struct AttemptEntry {
        std::vector<std::chrono::steady_clock::time_point> Timestamps;
    };

    struct CooldownEntry {
        std::chrono::steady_clock::time_point ExpiresAt;
    };

    std::mutex m_Mutex;
    std::unordered_map<std::string, AttemptEntry> m_AttemptMap;
    std::unordered_map<std::string, CooldownEntry> m_CooldownMap;

    void CleanupExpiredAttempts(AttemptEntry& entry, int windowSeconds);
    bool IsOnCooldown(const std::string& key);
};

std::string GetClientIp(const crow::request& req);