#pragma once
#include <crow.h>
#include <string>

inline std::string GetCookieValue(const crow::request& req, const std::string& cookieName) {
    std::string cookieHeader = req.get_header_value("Cookie");
    if (cookieHeader.empty()) return "";

    size_t pos = 0;
    while (pos < cookieHeader.size()) {
        size_t semi = cookieHeader.find(';', pos);
        std::string pair = cookieHeader.substr(pos, semi == std::string::npos ? std::string::npos : semi - pos);

        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = pair.substr(0, eq);
            size_t start = key.find_first_not_of(' ');
            if (start != std::string::npos) key = key.substr(start);
            if (key == cookieName) return pair.substr(eq + 1);
        }

        if (semi == std::string::npos) break;
        pos = semi + 1;
    }

    return "";
}