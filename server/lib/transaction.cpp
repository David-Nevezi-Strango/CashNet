#include "transaction.h"

Transaction::Transaction(int transaction_id, int account_id, int dest_account_id, int customer_id, double sum, int transaction_type, const std::string& date)
    : transaction_id(transaction_id), account_id(account_id), dest_account_id(dest_account_id), customer_id(customer_id), sum(sum), transaction_type(transaction_type), date(date) {
}

Transaction::Transaction(int account_id, int dest_account_id, int customer_id, double sum, int transaction_type, const std::string& date)
    : transaction_id(0), account_id(account_id), dest_account_id(dest_account_id), customer_id(customer_id), sum(sum), transaction_type(transaction_type), date(date) {
}

Transaction::Transaction(json js)
    : transaction_id(0), account_id(js["account_id"]), dest_account_id(js["dest_account_id"]), customer_id(js["customer_id"]), sum(js["sum"]), transaction_type(js["transaction_type"]), date(js["date"]) {
}

json Transaction::toJson() {
    return json{{"transaction_id", this->transaction_id},
                {"account_id", this->account_id},
                {"dest_account_id", this->dest_account_id},
                {"customer_id", this->customer_id},
                {"sum", this->sum},
                {"transaction_type", this->transaction_type},
                {"date", this->date}};
}

bool Transaction::insertIntoDatabase(sqlite3* db) {
    if (!checkIfOpen(db, this->account_id)){
        return false;
    }

    std::string sql = "INSERT INTO transactions (account_id, dest_account_id, customer_id, sum, transaction_type, date) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        // sqlite3_bind_int(statement, 1, this->transaction_id);
        sqlite3_bind_int(statement, 1, this->account_id);
        sqlite3_bind_int(statement, 2, this->dest_account_id);
        sqlite3_bind_int(statement, 3, this->customer_id);
        sqlite3_bind_double(statement, 4, this->sum);
        sqlite3_bind_int(statement, 5, this->transaction_type);
        sqlite3_bind_text(statement, 6, this->date.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(statement) != SQLITE_DONE) {
            std::cout << "Failed Insert!" << std::endl;
            return false;
        }

        sqlite3_finalize(statement);
        return true;
    }

    return false;
}

bool Transaction::checkIfOpen(sqlite3* db, int account_id) {
    std::string query = "SELECT COUNT(transaction_id) FROM transactions WHERE account_id = ? and transaction_type = 1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }
    
    rc = sqlite3_bind_int(stmt, 1, account_id);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to bind parameter: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }
    
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return !(count > 0);
    }
    
    return false;
    
}

json Transaction::getAllClosedAccountsID(sqlite3* db) {
    std::vector<int> transactions;
    std::string query = "SELECT account_id FROM transactions WHERE transaction_type = 1";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        while (sqlite3_step(statement) == SQLITE_ROW) {
            int account_id = sqlite3_column_int(statement, 0);

            transactions.emplace_back(account_id);
        }

        sqlite3_finalize(statement);
    }
    // json jsonArray;

    // for (auto &element : transactions) {
    //     json transactionJson = element.toJson();
    //     jsonArray.push_back(transactionJson);
    // }  
    json result;
    result["result"] = transactions;

    return result;
}

json Transaction::getAllTransactionByAccountID(sqlite3* db, int account_id) {
    std::vector<Transaction> transactions;
    std::string sql = "SELECT transaction_id, dest_account_id, customer_id, sum, transaction_type, date FROM transactions WHERE account_id = ?;";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(statement, 1, account_id);
        while (sqlite3_step(statement) == SQLITE_ROW) {
            int transaction_id = sqlite3_column_int(statement, 0);
            int dest_account_id = sqlite3_column_int(statement, 1);
            int customer_id = sqlite3_column_int(statement, 2);
            double sum = sqlite3_column_double(statement, 3);
            int transaction_type = sqlite3_column_int(statement, 4);
            const unsigned char* date = sqlite3_column_text(statement, 5);

            transactions.emplace_back(transaction_id, account_id, dest_account_id, customer_id, sum, transaction_type, reinterpret_cast<const char*>(date));
        }

        sqlite3_finalize(statement);
    }
    json jsonArray;

    for (auto &element : transactions) {
        json transactionJson = element.toJson();
        jsonArray.push_back(transactionJson);
    }  
    json result;
    result["result"] = jsonArray;

    return result;
}

json Transaction::getAllTransactionByCustomerID(sqlite3* db, int customer_id) {
    std::vector<Transaction> transactions;
    std::string sql = "SELECT transaction_id, account_id, dest_account_id, sum, transaction_type, date FROM transactions WHERE customer_id = ?;";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(statement, 1, customer_id);
        while (sqlite3_step(statement) == SQLITE_ROW) {
            int transaction_id = sqlite3_column_int(statement, 0);
            int account_id = sqlite3_column_int(statement, 1);
            int dest_account_id = sqlite3_column_int(statement, 2);
            double sum = sqlite3_column_double(statement, 3);
            int transaction_type = sqlite3_column_int(statement, 4);
            const unsigned char* date = sqlite3_column_text(statement, 5);

            transactions.emplace_back(transaction_id, account_id, dest_account_id, customer_id, sum, transaction_type, reinterpret_cast<const char*>(date));
        }

        sqlite3_finalize(statement);
    }

    json jsonArray;

    for (auto &element : transactions) {
        json transactionJson = element.toJson();
        jsonArray.push_back(transactionJson);
    }  
    json result;
    result["response"] = jsonArray;

    return result;
}