#include "customer.h"


Customer::Customer(int customer_id, const std::string& customer_name, const std::string& customer_password)
    : customer_id(customer_id), customer_name(customer_name), customer_password(customer_password) {
}

Customer::Customer(json js)
    : customer_id(0), customer_name(js["customer_name"]), customer_password(js["customer_password"]) {
}
json Customer::toJson() {
    //helper function to put object into json
    return json{{"customer_id", this->customer_id},
                {"customer_name", this->customer_name},
                {"customer_password", this->customer_password}};
}
int Customer::insertIntoDatabase(sqlite3* db) {
    if(!Customer::checkName(db)){
        //if customer does not exist, insert it into DB
        std::string sql = "INSERT INTO customers (customer_name, customer_password) VALUES (?, ?);";
        sqlite3_stmt* statement;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(statement, 1, this->customer_name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(statement, 2, this->customer_password.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(statement) != SQLITE_DONE) {
                std::cout << "Failed Insert!" << std::endl;
                return 0;
            }

            sqlite3_finalize(statement);
            //log new customer in to get it's ID
            json loginID = Customer::login(db, customer_name, customer_password);
            return loginID["id"];
        }

        
    }
    return 0;
}

bool Customer::checkName(sqlite3* db) {
    //helper function to check if this object exists in DB
    std::string query = "SELECT COUNT(*) FROM customers WHERE customer_name = ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }
    
    rc = sqlite3_bind_text(stmt, 1, this->customer_name.c_str(), -1, SQLITE_STATIC);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to bind parameter: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }
    
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return (count > 0);
    }
    
    return false;
    
}

json Customer::login(sqlite3* db, std::string customer_name, std::string customer_password ){
    //function to log customer in
    std::string sql = "SELECT customer_id FROM customers WHERE customer_name = ? AND customer_password = ?;";
    sqlite3_stmt* statement;
    json result;
    int customer_id = -1;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(statement, 1, customer_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(statement, 2, customer_password.c_str(), -1, SQLITE_STATIC);
        
        while (sqlite3_step(statement) == SQLITE_ROW) {
            customer_id = sqlite3_column_int(statement, 0);
        }
        

        sqlite3_finalize(statement);
    } 

    result["id"] = customer_id;

    return result;
}

json Customer::getAllCustomer(sqlite3* db) {
    //function to get all customers
    std::vector<Customer> customers;
    std::string sql = "SELECT customer_id, customer_name, customer_password FROM customers;";
    sqlite3_stmt* statement;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        while (sqlite3_step(statement) == SQLITE_ROW) {
            int customer_id = sqlite3_column_int(statement, 0);
            const unsigned char* customer_name = sqlite3_column_text(statement, 1);
            const unsigned char* customer_password = sqlite3_column_text(statement, 2);

            customers.emplace_back(customer_id, reinterpret_cast<const char*>(customer_name), reinterpret_cast<const char*>(customer_password));
        }

        sqlite3_finalize(statement);
    }

    json jsonArray;

    for (auto &element : customers) {
        json customersJson = element.toJson();
        jsonArray.push_back(customersJson);
    }
    json result;
    result["response"] = jsonArray;

    return result;
}

bool Customer::deleteCustomerById(sqlite3* db, int customer_id){
    //function to delete a customer
    std::string query = "DELETE FROM customers WHERE customer_customer_id = ?";
    sqlite3_stmt* statement;


    if (sqlite3_prepare_v2(db, query.c_str(), -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(statement, 1, customer_id);
        if (sqlite3_step(statement) != SQLITE_DONE) {
            return false;
        }

        sqlite3_finalize(statement);
        return true;
    }
    return false;
}