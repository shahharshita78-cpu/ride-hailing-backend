#include "UserRepository.h"
#include <drogon/drogon.h>

namespace repositories {

void UserRepository::createUser(const std::string& name, const std::string& email, const std::string& phone, const std::string& passwordHash, const std::string& role,
                                std::function<void(const std::string&)> onSuccess, std::function<void(const std::exception&)> onError) {
    LOG_INFO << "UserRepository: createUser called for email " << email;
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
        LOG_ERROR << "UserRepository: No DB client available!";
        onError(std::runtime_error("No DB client"));
        return;
    }
    LOG_INFO << "UserRepository: Acquired DB client, executing SQL...";

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

    dbClient->execSqlAsync(
        sql,
        [onSuccess, onError](const drogon::orm::Result& result) {
            LOG_INFO << "UserRepository: DB callback onSuccess triggered";
            if (result.empty()) {
                onError(std::runtime_error("Failed to insert user"));
                return;
            }
            onSuccess(result[0]["user_id"].as<std::string>());
        },
        [onError](const drogon::orm::DrogonDbException& e) {
            LOG_ERROR << "UserRepository: DB callback onError triggered! " << e.base().what();
            onError(e.base());
        },
        name, email, phone, passwordHash, role
    );
}

void UserRepository::getUserByEmail(const std::string& email,
                                    std::function<void(const Json::Value&)> onSuccess, std::function<void(const std::exception&)> onError) {
    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
        onError(std::runtime_error("No DB client"));
        return;
    }

    dbClient->execSqlAsync(
        "SELECT * FROM users WHERE email = $1",
        [dbClient, email, onSuccess, onError](const drogon::orm::Result& result) {
            if (result.empty()) {
                onSuccess(Json::Value());
                return;
            }
            
            Json::Value user;
            user["user_id"] = result[0]["user_id"].as<std::string>();
            user["password_hash"] = result[0]["password_hash"].as<std::string>();
            user["role"] = result[0]["role"].as<std::string>();
            user["name"] = result[0]["name"].as<std::string>();
            
            if (user["role"].asString() == "PASSENGER") {
                dbClient->execSqlAsync(
                    "SELECT passenger_id FROM passenger WHERE user_id = $1",
                    [user, onSuccess, onError](const drogon::orm::Result& pRes) mutable {
                        if (!pRes.empty()) user["passenger_id"] = pRes[0]["passenger_id"].as<std::string>();
                        onSuccess(user);
                    },
                    [onError](const drogon::orm::DrogonDbException& e) {
                        onError(e.base());
                    },
                    user["user_id"].asString()
                );
            } else {
                dbClient->execSqlAsync(
                    "SELECT driver_id FROM driver WHERE user_id = $1",
                    [user, onSuccess, onError](const drogon::orm::Result& dRes) mutable {
                        if (!dRes.empty()) user["driver_id"] = dRes[0]["driver_id"].as<std::string>();
                        onSuccess(user);
                    },
                    [onError](const drogon::orm::DrogonDbException& e) {
                        onError(e.base());
                    },
                    user["user_id"].asString()
                );
            }
        },
        [onError](const drogon::orm::DrogonDbException& e) {
            onError(e.base());
        },
        email
    );
}

}
