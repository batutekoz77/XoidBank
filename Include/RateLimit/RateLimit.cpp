#include "RateLimit.hpp"
#include <algorithm>

RateLimiter::RateLimiter() {}

void RateLimiter::CleanupExpiredAttempts(AttemptEntry& entry, int windowSeconds) {
    auto now = std::chrono::steady_clock::now();
    entry.Timestamps.erase(std::remove_if(entry.Timestamps.begin(), entry.Timestamps.end(), [&](const std::chrono::steady_clock::time_point& t) { return std::chrono::duration_cast<std::chrono::seconds>(now - t).count() >= windowSeconds; }), entry.Timestamps.end());
}

bool RateLimiter::IsOnCooldown(const std::string& key) {
    auto it = m_CooldownMap.find(key);
    if (it == m_CooldownMap.end()) return false;
    auto now = std::chrono::steady_clock::now();
    if (now >= it->second.ExpiresAt) {
        m_CooldownMap.erase(it);
        return false;
    }
    return true;
}

RateLimitResult RateLimiter::CheckRequest(const std::string& key, int maxAttempts, int windowSeconds, int cooldownSeconds) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (IsOnCooldown(key)) return RateLimitResult::OnCooldown;
    auto& entry = m_AttemptMap[key];
    CleanupExpiredAttempts(entry, windowSeconds);
    if (static_cast<int>(entry.Timestamps.size()) >= maxAttempts) {
        m_CooldownMap[key] = { std::chrono::steady_clock::now() + std::chrono::seconds(cooldownSeconds) };
        m_AttemptMap.erase(key);
        return RateLimitResult::LimitExceeded;
    }
    entry.Timestamps.push_back(std::chrono::steady_clock::now());
    return RateLimitResult::Allowed;
}

bool RateLimiter::CheckSimpleCooldown(const std::string& key, int cooldownSeconds) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (IsOnCooldown(key)) return false;
    m_CooldownMap[key] = { std::chrono::steady_clock::now() + std::chrono::seconds(cooldownSeconds) };
    return true;
}

int RateLimiter::GetRemainingCooldown(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_CooldownMap.find(key);
    if (it == m_CooldownMap.end()) return 0;
    auto now = std::chrono::steady_clock::now();
    if (now >= it->second.ExpiresAt) return 0;
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(it->second.ExpiresAt - now).count());
}

void RateLimiter::Reset(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_AttemptMap.erase(key);
    m_CooldownMap.erase(key);
}

std::string GetClientIp(const crow::request& req) {
    std::string forwardedFor = req.get_header_value("X-Forwarded-For");
    if (!forwardedFor.empty()) {
        size_t commaPos = forwardedFor.find(',');
        return commaPos != std::string::npos ? forwardedFor.substr(0, commaPos) : forwardedFor;
    }
    return req.remote_ip_address;
}