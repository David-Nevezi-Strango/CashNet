#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <vector>
#include <iostream>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// CREATED = 0,
// CLOSED = 1,
// WITHDRAW = 2,
// DEPOSIT = 3,
// PAYMENT_SENT = 4,
// PAYMENT_RECEIVED = 5,
// NUM_TYPES
        


class Transaction {
    
private:
    int transaction_id;
    int account_id;
    int dest_account_id;
    int customer_id;
    double sum;
    int transaction_type;
    std::string date;

public:
    
    Transaction(int transaction_id, int account_id, int dest_account_id, int customer_id, double sum, int transaction_type, const std::string& date);
    
    Transaction(int account_id, int dest_account_id, int customer_id, double sum, int transaction_type, const std::string& date);

    Transaction(json js);
    
    json toJson();

    bool insertIntoDatabase(sqlite3* db);

    static bool checkIfOpen(sqlite3* db, int account_id);

    static json getAllClosedAccountsID(sqlite3* db);

    static json getAllTransactionByAccountID(sqlite3* db, int account_id);

    static json getAllTransactionByCustomerID(sqlite3* db, int customer_id);
};

#endif