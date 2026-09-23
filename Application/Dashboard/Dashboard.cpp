#include "Dashboard.hpp"
#include "../../Main/Config.hpp"
#include "../../Include/CookieUtils.hpp"

void DashboardRouter::RegisterRoutes(CrowApp& app, UsersBase& usersBase, AccountsBase& accountsBase, SessionManager& sessionManager) {
    CROW_ROUTE(app, "/Dashboard/Data").methods(crow::HTTPMethod::GET) ([&usersBase, &accountsBase, &sessionManager](const crow::request& req) {
        std::string sessionId = GetCookieValue(req, "session_id");
        auto userId = sessionManager.ValidateSession(sessionId);

        if (!userId.has_value()) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INVALID_AUTHENTICATION_SESSION;
            return crow::response(Status::UNAUTHORIZED, response);
        }

        auto fullName = usersBase.getFullNameById(userId.value());

        if (!fullName.has_value()) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INTERNAL_SERVER_ERROR;
            return crow::response(Status::INTERNAL_SERVER_ERROR, response);
        }

        int accountId = accountsBase.EnsureAccountForUser(userId.value());

        if (accountId < 0) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INTERNAL_SERVER_ERROR;
            return crow::response(Status::INTERNAL_SERVER_ERROR, response);
        }

        auto accountInfo = accountsBase.GetAccountById(accountId);

        if (!accountInfo.has_value()) {
            crow::json::wvalue response;
            response["success"] = false;
            response["message"] = Message::INTERNAL_SERVER_ERROR;
            return crow::response(Status::INTERNAL_SERVER_ERROR, response);
        }

        auto transactions = accountsBase.GetRecentTransactions(accountId, 20);

        crow::json::wvalue response;
        response["success"] = true;
        response["user"]["fullName"] = fullName.value();
        response["account"]["balance"] = accountInfo->BalanceCents / 100.0;
        response["account"]["iban"] = accountInfo->Iban;

        std::vector<crow::json::wvalue> txArray;
        for (const auto& tx : transactions) {
            crow::json::wvalue item;
            item["id"] = tx.Id;
            item["description"] = tx.Description;
            item["amount"] = tx.AmountCents / 100.0;
            item["date"] = tx.DateLabel;
            txArray.push_back(std::move(item));
        }
        response["transactions"] = std::move(txArray);

        return crow::response(Status::SUCCESS, response);
    });
}