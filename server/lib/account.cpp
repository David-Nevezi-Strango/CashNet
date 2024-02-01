#include "account.h"
#include "transaction.h"
Account::Account(int account_id, int customer_id, double current_sum, const std::string& currency)
    : account_id(account_id), customer_id(customer_id), current_sum(current_sum), currency(currency){
}

Account::Account(json js)
    : account_id(js["account_id"]), customer_id(js["customer_id"]), current_sum(js["current_sum"]), currency(js["currency"]) {
}

json Account::toJson() {
    return json{{"account_id", this->account_id},
                {"customer_id", this->customer_id},
                {"current_sum", this->current_sum},
                {"currency", this->currency}};
}

bool Account::insertIntoDatabase(sqlite3* db) {
    std::string sql = "INSERT INTO accounts (account_id, customer_id, current_sum, currency) VALUES (?, ?, ?, ?);";
        sqlite3_stmt* statement;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(statement, 1, this->account_id);
            sqlite3_bind_int(statement, 2, this->customer_id);
            sqlite3_bind_int(statement, 3, this->current_sum);
            sqlite3_bind_text(statement, 4, this->currency.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(statement) != SQLITE_DONE) {
                std::cout << "Failed Insert!" << std::endl;
                return false;
            }

            sqlite3_finalize(statement);
            return true;
        }

        return false;
}
bool Account::updateAccount(sqlite3* db, int account_id, double sum){
    std::string sql = "UPDATE accounts SET current_sum = current_sum + ? WHERE account_id = ?;";
    sqlite3_stmt* statement;
    
    if(Transaction::checkIfOpen(db, account_id)){
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
            sqlite3_bind_double(statement, 1, sum);
            sqlite3_bind_int(statement, 2, account_id);
            int rc = sqlite3_step(statement);
            if (rc != SQLITE_DONE) {
                    std::cout << "Failed Update!" << std::endl;
                    return false;
            }

            sqlite3_finalize(statement);
            return true;
        }
    }
    return false;
}


json Account::getAllAccounts(sqlite3* db) {
        std::vector<Account> accounts;
        std::string sql = "SELECT account_id, customer_id , current_sum, currency FROM accounts;";
        sqlite3_stmt* statement;
    
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
            
            while (sqlite3_step(statement) == SQLITE_ROW) {
                int account_id = sqlite3_column_int(statement, 0);
                int customer_id = sqlite3_column_int(statement, 1);
                double current_sum = sqlite3_column_double(statement, 2);
                const unsigned char* currency = sqlite3_column_text(statement, 3);
                if(Transaction::checkIfOpen(db, account_id)){
                    accounts.emplace_back(account_id, customer_id, current_sum, reinterpret_cast<const char*>(currency));
                }
            }

            sqlite3_finalize(statement);
        }

        json jsonArray;

        for (auto &element : accounts) {
            json accountJson = element.toJson();
            jsonArray.push_back(accountJson);
        }  
        json result;
        result["response"] = jsonArray;
        return result;
    }

json Account::getAccountByID(sqlite3* db, int account_id){

    std::string sql = "SELECT customer_id, current_sum, currency FROM accounts WHERE account_id = ?;";
    sqlite3_stmt* statement;
    json result;

    if(Transaction::checkIfOpen(db, account_id)){
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(statement, 1, account_id);
            if (sqlite3_step(statement) == SQLITE_ROW) {
                int customer_id = sqlite3_column_int(statement, 0);
                double current_sum = sqlite3_column_double(statement, 1);
                const unsigned char* currency = sqlite3_column_text(statement, 2);
                Account account(account_id, customer_id, current_sum, reinterpret_cast<const char*>(currency));
                result["response"] = account.toJson();
            }
            sqlite3_finalize(statement);
        }
    }
    return result;
}

json Account::getLastAccountID(sqlite3* db){
    std::string sql = "SELECT MAX(account_id) FROM accounts;";
    sqlite3_stmt* statement;
    int account_id = 0;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        if (sqlite3_step(statement) == SQLITE_ROW) {
            account_id = sqlite3_column_int(statement, 0);
            account_id++;
        }

        sqlite3_finalize(statement);
    }

    json result;
    result["response"] = account_id;
    return result;
}

json Account::getAllAccountsByCustomerID(sqlite3* db, int customer_id){
    std::vector<Account> accounts;
    std::string sql = "SELECT account_id, customer_id , current_sum, currency FROM accounts WHERE customer_id = ?;";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(statement, 1, customer_id);
         
        while (sqlite3_step(statement) == SQLITE_ROW) {
            int account_id = sqlite3_column_int(statement, 0);
            int customer_id = sqlite3_column_int(statement, 1);
            double current_sum = sqlite3_column_double(statement, 2);
            const unsigned char* currency = sqlite3_column_text(statement, 3);
            if(Transaction::checkIfOpen(db, account_id)){
                accounts.emplace_back(account_id, customer_id, current_sum, reinterpret_cast<const char*>(currency));
            }

        }

        sqlite3_finalize(statement);
    }

    json jsonArray;

    for (auto &element : accounts) {
        json accountJson = element.toJson();
        jsonArray.push_back(accountJson);
    }  
    json result;
    result["response"] = jsonArray;
    return result;
}


// bool Customer::deleteAccountById(sqlite3* db, int account_id){
//     std::string query = "DELETE FROM accounts WHERE account_id = ?";
//     sqlite3_stmt* statement;


//     if (sqlite3_prepare_v2(db, query.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
//         sqlite3_bind_int(statement, 1, account_id);
//         if (sqlite3_step(statement) != SQLITE_DONE) {
//             return false;
//         }

//         sqlite3_finalize(statement);
//         return true;
//     }
//     return false;
// }