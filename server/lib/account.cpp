#include "account.h"
#include "transaction.h"
Account::Account(int account_id, double current_sum, const std::string& currency)
    : account_id(account_id), current_sum(current_sum), currency(currency){
}

Account::Account(double current_sum, const std::string& currency)
    : account_id(0), current_sum(current_sum), currency(currency){
}
Account::Account(json js)
    : account_id(0), current_sum(js["current_sum"]), currency(js["currency"]) {
}

json Account::toJson() {
    //helper function to put object into json
    return json{{"account_id", this->account_id},
                {"current_sum", this->current_sum},
                {"currency", this->currency}};
}

int Account::insertIntoDatabase(sqlite3* db) {
    //function to insert object into DB
    int account_id = -1;
    std::string sql = "INSERT INTO accounts (current_sum, currency) VALUES (?, ?) RETURNING account_id;";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(statement, 1, this->current_sum);
        sqlite3_bind_text(statement, 2, this->currency.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(statement) == SQLITE_ROW) {
            account_id = sqlite3_column_int(statement, 0);
            // if(!Account::postAccountConnection(account_id, customer_id)) {
            //     std::cout << "Failed Account Insert!" << std::endl;
            //     return false;
            // }   
        }

        sqlite3_finalize(statement);
        // return true;
    }

    return account_id;
}
bool Account::updateAccount(sqlite3* db, int account_id, double sum){
    //function to update sum on the account
    std::string sql = "UPDATE accounts SET current_sum = current_sum + ? WHERE account_id = ?;";
    sqlite3_stmt* statement;
    
    if(Transaction::checkIfOpen(db, account_id)){
        //if account was not closed, update
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


bool Account::postAccountConnection(sqlite3* db, int account_id, int customer_id){
    //function to add connection to an account
    std::string sql = "INSERT INTO account_connections (account_id, customer_id) VALUES (?, ?);";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(statement, 1, account_id);
        sqlite3_bind_int(statement, 2, customer_id);

        if (sqlite3_step(statement) != SQLITE_DONE) {
            std::cout << "Failed Connection Insert!" << std::endl;
            return false;
        }

        sqlite3_finalize(statement);
        return true;
    }

    return false;
}


json Account::getAllAccounts(sqlite3* db) {
    //function to get all accounts
    std::vector<Account> accounts;
    std::string sql = "SELECT account_id, current_sum, currency FROM accounts;";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        
        while (sqlite3_step(statement) == SQLITE_ROW) {
            int account_id = sqlite3_column_int(statement, 0);
            double current_sum = sqlite3_column_double(statement, 1);
            const unsigned char* currency = sqlite3_column_text(statement, 2);
            if(Transaction::checkIfOpen(db, account_id)){
                //if account was not closed, add it to the list
                accounts.emplace_back(account_id, current_sum, reinterpret_cast<const char*>(currency));
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
    //function to get an account by it's ID
    std::string sql = "SELECT current_sum, currency FROM accounts WHERE account_id = ?;";
    sqlite3_stmt* statement;
    json result;

    if(Transaction::checkIfOpen(db, account_id)){
        //if account was not closed, get it
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(statement, 1, account_id);
            if (sqlite3_step(statement) == SQLITE_ROW) {
                // int customer_id = sqlite3_column_int(statement, 0);
                double current_sum = sqlite3_column_double(statement, 0);
                const unsigned char* currency = sqlite3_column_text(statement, 1);
                Account account(account_id, current_sum, reinterpret_cast<const char*>(currency));
                result["response"] = account.toJson();
            }
            sqlite3_finalize(statement);
        }
    }
    return result;
}

// int Account::getLastAccountID(sqlite3* db, int customer_id){
//     // function to increment account ID
//     std::string sql = "SELECT MAX(accounts.account_id) FROM accounts INNER JOIN account_connections ON accounts.account_id = account_connections.account_id WHERE account_connections.customer_id = ?;";
//     sqlite3_stmt* statement;
//     int account_id = -1;
//     if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
//         sqlite3_bind_int(statement, 1, customer_id);

//         if (sqlite3_step(statement) == SQLITE_ROW) {
//             account_id = sqlite3_column_int(statement, 0);
//         }

//         sqlite3_finalize(statement);
//     }

//     return account_id;
// }

json Account::getAllAccountsByCustomerID(sqlite3* db, int customer_id){
    //get a customer's accounts
    std::vector<Account> accounts;
    std::string sql = "SELECT accounts.account_id, current_sum, currency FROM accounts INNER JOIN account_connections ON accounts.account_id = account_connections.account_id WHERE account_connections.customer_id = ?;";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(statement, 1, customer_id);
         
        while (sqlite3_step(statement) == SQLITE_ROW) {
            int account_id = sqlite3_column_int(statement, 0);
            // int customer_id = sqlite3_column_int(statement, 1);
            double current_sum = sqlite3_column_double(statement, 1);
            const unsigned char* currency = sqlite3_column_text(statement, 2);
            if(Transaction::checkIfOpen(db, account_id)){
                //if account was not closed, add it to the list
                accounts.emplace_back(account_id, current_sum, reinterpret_cast<const char*>(currency));
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