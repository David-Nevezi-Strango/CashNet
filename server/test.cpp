#include <iostream>
#include <vector>
#include "nlohmann/json.hpp"

using json = nlohmann::json;


int main(){
    std::vector<int> c_vector {1, 2, 3, 4};
    //json j_vec(c_vector);
    // [1, 2, 3, 4]
    std::vector<json> response;
    json j_vec, j_val1, j_val2;
    j_val1["id"] = 1;
    j_val1["username"] = "marian";
    j_val1["varsta"] = 19;
    response.push_back(j_val1);
    j_vec.push_back(j_val1);
    j_val2["id"] = 1;
    j_val2["username"] = "marian";
    j_val2["varsta"] = 19;
    response.push_back(j_val2);
    j_vec.push_back(j_val1);
    // j_vec["response"] = response;
    std::cout << j_vec.dump(4) << std::endl;
    std::string s = j_vec.dump(); 
    auto j = json::parse(s);
    std::cout << j.dump() << std::endl;
    // std::cout << j[0] << std::endl;
    std::cout << j["response"] << std::endl;
    
}