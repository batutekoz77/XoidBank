#include "Accounts.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

AccountsBase::AccountsBase(const std::string& dbPath) : db(dbPath) {
    try {
        db << "CREATE TABLE IF NOT EXISTS Accounts ("
            "AccountId    INTEGER PRIMARY KEY AUTOINCREMENT,"
            "UserId       INTEGER NOT NULL UNIQUE,"
            "IBAN         TEXT NOT NULL UNIQUE,"
            "BalanceCents INTEGER NOT NULL DEFAULT 0,"
            "CreatedAt    TEXT NOT NULL DEFAULT (datetime('now')),"
            "FOREIGN KEY (UserId) REFERENCES Users(UserId)"
        ");";

        db << "CREATE TABLE IF NOT EXISTS Transactions ("
            "TransactionId INTEGER PRIMARY KEY AUTOINCREMENT,"
            "AccountId     INTEGER NOT NULL,"
            "Description   TEXT NOT NULL,"
            "AmountCents   INTEGER NOT NULL,"
            "CreatedAt     TEXT NOT NULL DEFAULT (datetime('now')),"
            "FOREIGN KEY (AccountId) REFERENCES Accounts(AccountId)"
        ");";
    }
    catch (const std::exception& e) { std::cerr << "Accounts DB initialization error: " << e.what() << std::endl; }
}

AccountsBase::~AccountsBase() {}

std::string AccountsBase::GenerateIban(int accountId) {
    std::ostringstream oss;
    oss << "NL00XOID" << std::setw(10) << std::setfill('0') << accountId;
    return oss.str();
}

std::optional<AccountInfo> AccountsBase::GetAccountByUserId(int userId) {
    AccountInfo info{};
    bool found = false;

    try {
        db << "SELECT AccountId, IBAN, BalanceCents FROM Accounts WHERE UserId = ?;"
            << userId
            >> [&](int accountId, std::string iban, long long balanceCents) {
            info.AccountId = accountId;
            info.Iban = iban;
            info.BalanceCents = balanceCents;
            found = true;
        };
    }
    catch (const std::exception& e) {
        std::cerr << "GetAccountByUserId error: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (!found) return std::nullopt;
    return info;
}

std::optional<AccountInfo> AccountsBase::GetAccountById(int accountId) {
    AccountInfo info{};
    bool found = false;

    try {
        db << "SELECT AccountId, IBAN, BalanceCents FROM Accounts WHERE AccountId = ?;"
            << accountId
            >> [&](int id, std::string iban, long long balanceCents) {
            info.AccountId = id;
            info.Iban = iban;
            info.BalanceCents = balanceCents;
            found = true;
        };
    }
    catch (const std::exception& e) {
        std::cerr << "GetAccountById error: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (!found) return std::nullopt;
    return info;
}

int AccountsBase::EnsureAccountForUser(int userId) {
    auto existing = GetAccountByUserId(userId);
    if (existing.has_value()) return existing->AccountId;
    
    try {
        db << "INSERT INTO Accounts (UserId, IBAN, BalanceCents) VALUES (?, ?, 0);"
            << userId << "";

        int newAccountId = static_cast<int>(db.last_insert_rowid());

        std::string iban = GenerateIban(newAccountId);
        db << "UPDATE Accounts SET IBAN = ? WHERE AccountId = ?;" << iban << newAccountId;

        return newAccountId;
    }
    catch (const std::exception& e) {
        std::cerr << "EnsureAccountForUser error: " << e.what() << std::endl;
        return -1;
    }
}

std::vector<TransactionInfo> AccountsBase::GetRecentTransactions(int accountId, int limit) {
    std::vector<TransactionInfo> results;

    try {
        db << "SELECT TransactionId, Description, AmountCents, "
            "CAST(julianday('now') - julianday(CreatedAt) AS INTEGER) AS DaysAgo, "
            "CreatedAt "
            "FROM Transactions "
            "WHERE AccountId = ? "
            "ORDER BY CreatedAt DESC "
            "LIMIT ?;"
            << accountId << limit >> [&](int transactionId, std::string description, long long amountCents, int daysAgo, std::string createdAt) {
                TransactionInfo tx;

                tx.Id = transactionId;
                tx.Description = description;
                tx.AmountCents = amountCents;

                if (daysAgo <= 0) tx.DateLabel = "Today";
                else if (daysAgo == 1) tx.DateLabel = "Yesterday";
                else tx.DateLabel = createdAt.substr(0, 10);

                results.push_back(tx);
            };
    }
    catch (const std::exception& e) { std::cerr << "GetRecentTransactions error: " << e.what() << std::endl; }
    return results;
}