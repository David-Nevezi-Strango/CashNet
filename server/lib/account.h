#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <string>
#include <vector>
#include <iostream>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class Account {
private:
    int account_id;
    int customer_id;
    double current_sum;
    std::string currency;

public:
    Account(int account_id, int customer_id, double current_sum, const std::string& currency);

    Account(json js);

    json toJson();

    bool insertIntoDatabase(sqlite3* db);

    static bool updateAccount(sqlite3* db, int account_id, double sum);

    static json getAccountByID(sqlite3* db, int account_id);

    static json getLastAccountID(sqlite3* db);
    
    static json getAllAccounts(sqlite3* db);

    static json getAllAccountsByCustomerID(sqlite3* db, int customer_id);

    // static bool deleteAccountById(sqlite3* db, int customer_id);
};

#endif