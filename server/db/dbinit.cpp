#include <iostream>
#include <sqlite3.h>
#include <string>

int main() {
    sqlite3* db;
    int result = sqlite3_open("database.db", &db);
    if (result != SQLITE_OK) {
        std::cout << "Error on creating the DB: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    std::string createTableSQL = "CREATE TABLE IF NOT EXISTS customers (customer_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, customer_name TEXT UNIQUE, customer_password TEXT);";
    char* errorMsg;
    result = sqlite3_exec(db, createTableSQL.c_str(), nullptr, nullptr, &errorMsg);
    if (result != SQLITE_OK) {
        std::cout << "Error on table creation: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        return 1;
    }

     createTableSQL = "CREATE TABLE IF NOT EXISTS accounts (account_id INTEGER NOT NULL, customer_id INTEGER NOT NULL, current_sum REAL NOT NULL, currency TEXT NOT NULL, FOREIGN KEY (customer_id) REFERENCES customers(customer_id), PRIMARY KEY (account_id, customer_id));";
    
    result = sqlite3_exec(db, createTableSQL.c_str(), nullptr, nullptr, &errorMsg);
    if (result != SQLITE_OK) {
        std::cout << "Error on table creation: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        return 1;
    }

     createTableSQL = "CREATE TABLE IF NOT EXISTS transactions (transaction_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, account_id INTEGER NOT NULL, dest_account_id INTEGER NOT NULL, customer_id INTEGER NOT NULL, sum REAL NOT NULL, transaction_type INTEGER NOT NULL, date TEXT NOT NULL, FOREIGN KEY(customer_id) REFERENCES customers(customer_id), FOREIGN KEY(account_id) REFERENCES accounts(account_id));";
    
    result = sqlite3_exec(db, createTableSQL.c_str(), nullptr, nullptr, &errorMsg);
    if (result != SQLITE_OK) {
        std::cout << "Error on table creation: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        return 1;
    }


    // Închiderea conexiunii cu baza de date
    sqlite3_close(db);

    return 0;
}