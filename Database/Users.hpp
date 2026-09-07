#pragma once
#include <sqlite_modern_cpp.h>
#include <string>

enum class LoginResult : int {
    Success = 0,
    WrongPassword = 1,
    AlreadyCreated = 2,
    AccountInactive = 3,
    EmailAlreadyExists = 4,
    PhoneAlreadyExists = 5,
    BSNAlreadyExists = 6,
    DatabaseError = 999
};

class LoginResultMessages {
public:
    static std::string ToString(LoginResult result) {
        switch (result) {
            case LoginResult::Success:                 return "Login successful";
            case LoginResult::WrongPassword:           return "Invalid email or password";
            case LoginResult::AlreadyCreated:          return "An account with this information already exists";
            case LoginResult::AccountInactive:         return "Your account has been suspended, please contact support";
            case LoginResult::EmailAlreadyExists:      return "An account with this email already exists";
            case LoginResult::PhoneAlreadyExists:      return "An account with this phone number already exists";
            case LoginResult::BSNAlreadyExists:        return "An account with this BSN already exists";
            case LoginResult::DatabaseError:           return "Server error, please try again later";
            default:                                   return "Unknown error";
        }
    }

    static std::string ToString(int resultCode) { return ToString(static_cast<LoginResult>(resultCode)); }
};

class UsersBase {
public:
    UsersBase(const std::string& dbPath);
    ~UsersBase();

    int addUser(const std::string& fullName, const std::string& email, const std::string& password, const std::string& phoneNumber, const std::string& BSN);
    int validateLogin(const std::string& email, const std::string& password);
    
    bool emailExists(const std::string& email);
    bool phoneExists(const std::string& phoneNumber);
    bool BSNExists(const std::string& BSN);

    std::optional<int> getUserIdByEmail(const std::string& email);
    std::optional<std::string> getEmailById(int userId);
private:
    sqlite::database db;

    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
};