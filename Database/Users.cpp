#include "Users.hpp"

#include <sodium.h>
#include "string"
#include <iostream>
#include <sstream>

UsersBase::UsersBase(const std::string& dbPath) : db(dbPath) {
    try {
        db << "CREATE TABLE IF NOT EXISTS Users ("
            "UserId          INTEGER PRIMARY KEY AUTOINCREMENT,"
            "FullName        TEXT NOT NULL,"
            "Email           TEXT NOT NULL UNIQUE,"
            "PasswordHash    TEXT NOT NULL,"
            "PhoneNumber     TEXT NOT NULL UNIQUE,"
            "BSN             TEXT NOT NULL UNIQUE,"
            "Role            TEXT NOT NULL DEFAULT 'customer',"
            "IsActive        INTEGER NOT NULL DEFAULT 1," /* @note: if suspended account 0 */
            "IsEmailVerified INTEGER NOT NULL DEFAULT 0,"
            "CreatedAt       TEXT NOT NULL DEFAULT (datetime('now')),"
            "LastLoginAt     TEXT"
        ");";
    }
    catch (const std::exception& e) { std::cerr << "DB initialization error: " << e.what() << std::endl; }
}

UsersBase::~UsersBase() {

}

std::string UsersBase::hashPassword(const std::string& password) {
    char hashedBuffer[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(hashedBuffer, password.c_str(), password.size(), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) throw std::runtime_error("An error occuped in the memory while hashing the password");
    return std::string(hashedBuffer);
}

bool UsersBase::verifyPassword(const std::string& password, const std::string& hash) {
    return crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.size()) == 0;
}

bool UsersBase::emailExists(const std::string& email) {
    bool found = false;

    db << "SELECT 1 FROM Users WHERE Email = ?;"
        << email
        >> [&]() { found = true; };

    return found;
}

bool UsersBase::phoneExists(const std::string& phoneNumber) {
    bool found = false;
    db << "SELECT 1 FROM Users WHERE PhoneNumber = ?;" << phoneNumber >> [&]() { found = true; };
    return found;
}

bool UsersBase::BSNExists(const std::string& BSN) {
    bool found = false;
    db << "SELECT 1 FROM Users WHERE BSN = ?;" << BSN >> [&]() { found = true; };
    return found;
}

std::optional<int> UsersBase::getUserIdByEmail(const std::string& email) {
    int userId = -1;
    bool found = false;

    try {
        db << "SELECT UserId FROM Users WHERE Email = ?;"
            << email
            >> [&](int id) {
            userId = id;
            found = true;
        };
    }
    catch (const std::exception& e) {
        std::cerr << "getUserIdByEmail error: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (!found) return std::nullopt;
    return userId;
}

std::optional<std::string> UsersBase::getEmailById(int userId) {
    std::string email;
    bool found = false;

    try {
        db << "SELECT Email FROM Users WHERE UserId = ?;"
            << userId
            >> [&](std::string e) { email = e; found = true; };
    }
    catch (const std::exception& e) {
        std::cerr << "getEmailById error: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (!found) return std::nullopt;
    return email;
}

int UsersBase::addUser(const std::string& fullName, const std::string& email, const std::string& password,
    const std::string& phoneNumber, const std::string& BSN) {
    if (emailExists(email)) return static_cast<int>(LoginResult::AlreadyCreated);
    if (phoneExists(phoneNumber)) return static_cast<int>(LoginResult::PhoneAlreadyExists);
    if (BSNExists(BSN)) return static_cast<int>(LoginResult::BSNAlreadyExists);

    try {
        std::string hashed = hashPassword(password);

        db << "INSERT INTO Users (FullName, Email, PasswordHash, PhoneNumber, BSN) "
            "VALUES (?, ?, ?, ?, ?);"
            << fullName << email << hashed << phoneNumber << BSN;

        return static_cast<int>(LoginResult::Success);
    }
    catch (const std::exception& e) {
        std::cerr << "addUser error: " << e.what() << std::endl;
        return static_cast<int>(LoginResult::DatabaseError);
    }
}

int UsersBase::validateLogin(const std::string& email, const std::string& password) {
    std::string storedHash;
    bool active = false;
    bool found = false;

    try {
        db << "SELECT PasswordHash, IsActive FROM Users WHERE Email = ?;"
            << email
            >> [&](std::string hash, int isActive) {
            storedHash = hash;
            active = (isActive != 0);
            found = true;
        };
    }
    catch (const std::exception& e) {
        std::cerr << "validateLogin DB error: " << e.what() << std::endl;
        return static_cast<int>(LoginResult::DatabaseError);
    }

    if (!found) return static_cast<int>(LoginResult::WrongPassword);
    if (!active) return static_cast<int>(LoginResult::AccountInactive);
    if (!verifyPassword(password, storedHash)) return static_cast<int>(LoginResult::WrongPassword);
    try { db << "UPDATE Users SET LastLoginAt = datetime('now') WHERE Email = ?;" << email; }
    catch (const std::exception& e) { std::cerr << "LastLoginAt update error: " << e.what() << std::endl; }
    return static_cast<int>(LoginResult::Success);
}