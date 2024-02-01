#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <string>
#include <vector>
#include <iostream>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

class Customer {
private:
    int customer_id;
    std::string customer_name;
    std::string customer_password;

public:
    
    Customer(int customer_id, const std::string& customer_name, const std::string& customer_password);

    Customer(json js);

    json toJson();

    int insertIntoDatabase(sqlite3* db);

    static json login(sqlite3* db, std::string customer_name, std::string customer_password);

    bool checkName(sqlite3* db);

    static bool deleteCustomerById(sqlite3* db, int customer_id);
    
    static json getAllCustomer(sqlite3* db);
};

#endif