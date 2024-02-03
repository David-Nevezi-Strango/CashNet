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
    double current_sum;
    std::string currency;

public:
    Account(int account_id, double current_sum, const std::string& currency);

    Account(double current_sum, const std::string& currency);

    Account(json js);

    json toJson();

    int insertIntoDatabase(sqlite3* db);

    static bool updateAccount(sqlite3* db, int account_id, double sum);

    static bool postAccountConnection(sqlite3* db, int account_id, int customer_id);

    static json getAccountByID(sqlite3* db, int account_id);
    
    static json getAllAccounts(sqlite3* db);

    static json getAllAccountsByCustomerID(sqlite3* db, int customer_id);

};

#endif