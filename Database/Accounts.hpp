#pragma once
#include <sqlite_modern_cpp.h>
#include <string>
#include <vector>
#include <optional>

struct AccountInfo {
    int AccountId;
    std::string Iban;
    long long BalanceCents;
};

struct TransactionInfo {
    int Id;
    std::string Description;
    long long AmountCents;
    std::string DateLabel;
};

class AccountsBase {
public:
    AccountsBase(const std::string& dbPath);
    ~AccountsBase();

    int EnsureAccountForUser(int userId);

    std::optional<AccountInfo> GetAccountById(int accountId);
    std::optional<AccountInfo> GetAccountByUserId(int userId);

    std::vector<TransactionInfo> GetRecentTransactions(int accountId, int limit);

private:
    sqlite::database db;

    std::string GenerateIban(int accountId);
};