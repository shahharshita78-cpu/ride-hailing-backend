#include "UserRepository.h"
#include <drogon/drogon.h>

namespace repositories {

std::string UserRepository::createUser(const std::string& name, const std::string& email, const std::string& phone, const std::string& passwordHash, const std::string& role) {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) throw std::runtime_error("No DB client");

    std::string sql = R"(
        WITH new_user AS (
            INSERT INTO users (name, email, phone, password_hash, role) 
            VALUES ($1, $2, $3, $4, CAST($5 AS user_role)) 
            RETURNING user_id, role
        ), new_passenger AS (
            INSERT INTO passenger (user_id)
            SELECT user_id FROM new_user WHERE role = 'PASSENGER'
        ), new_driver AS (
            INSERT INTO driver (user_id)
            SELECT user_id FROM new_user WHERE role = 'DRIVER'
        )
        SELECT user_id FROM new_user;
    )";

    auto result = dbClient->execSqlSync(sql, name, email, phone, passwordHash, role);
    
    if (result.empty()) {
        throw std::runtime_error("Failed to insert user");
    }
    
    return result[0]["user_id"].as<std::string>();
}

Json::Value UserRepository::getUserByEmail(const std::string& email) {
    auto dbClient = drogon::app().getDbClient();
    auto result = dbClient->execSqlSync("SELECT * FROM users WHERE email = $1", email);
    
    if (result.empty()) return Json::Value();
    
    Json::Value user;
    user["user_id"] = result[0]["user_id"].as<std::string>();
    user["password_hash"] = result[0]["password_hash"].as<std::string>();
    user["role"] = result[0]["role"].as<std::string>();
    user["name"] = result[0]["name"].as<std::string>();
    
    if (user["role"].asString() == "PASSENGER") {
        auto pRes = dbClient->execSqlSync("SELECT passenger_id FROM passenger WHERE user_id = $1", user["user_id"].asString());
        if (!pRes.empty()) user["passenger_id"] = pRes[0]["passenger_id"].as<std::string>();
    } else {
        auto dRes = dbClient->execSqlSync("SELECT driver_id FROM driver WHERE user_id = $1", user["user_id"].asString());
        if (!dRes.empty()) user["driver_id"] = dRes[0]["driver_id"].as<std::string>();
    }
    
    return user;
}

}
