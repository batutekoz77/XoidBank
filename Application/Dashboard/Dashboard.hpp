#pragma once
#include "../../Include/Crow.hpp"
#include "../../Database/Users.hpp"
#include "../../Database/Accounts.hpp"
#include "../../Session/Session.hpp"

class DashboardRouter {
public:
    static void RegisterRoutes(CrowApp& app, UsersBase& usersBase, AccountsBase& accountsBase, SessionManager& sessionManager);
};