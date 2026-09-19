#include "UserController.h"
#include "../utils/CryptoUtils.h"
#include "../utils/JwtUtils.h"
#include <drogon/orm/DbClient.h>

using namespace api::v1;

void UserController::signup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing JSON body");
        callback(resp);
        return;
    }

    auto& json = *jsonPtr;
    if (!json.isMember("email") || !json.isMember("password") || !json.isMember("role")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing email, password, or role");
        callback(resp);
        return;
    }

    std::string email = json["email"].asString();
    std::string password = json["password"].asString();
    std::string role = json["role"].asString();

    if (role != "rider" && role != "driver") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Role must be rider or driver");
        callback(resp);
        return;
    }

    std::string salt = utils::crypto::generateSalt();
    std::string passwordHash = utils::crypto::hashPassword(password, salt);

    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database connection failed");
        callback(resp);
        return;
    }

    try {
        auto result = dbClient->execSqlSync("SELECT id FROM users WHERE email = $1", email);
        if (!result.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k409Conflict);
            resp->setBody("User already exists");
            callback(resp);
            return;
        }

        auto insertResult = dbClient->execSqlSync(
            "INSERT INTO users (email, password_hash, role) VALUES ($1, $2, $3) RETURNING id",
            email, passwordHash, role
        );

        if (!insertResult.empty()) {
            std::string userId = insertResult[0]["id"].as<std::string>();
            std::string token = utils::jwt_utils::generateToken(userId, role);

            Json::Value ret;
            ret["message"] = "Signup successful";
            ret["token"] = token;
            ret["user_id"] = userId;

            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Failed to create user");
            callback(resp);
        }
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database error");
        callback(resp);
    }
}

void UserController::login(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing JSON body");
        callback(resp);
        return;
    }

    auto& json = *jsonPtr;
    if (!json.isMember("email") || !json.isMember("password")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing email or password");
        callback(resp);
        return;
    }

    std::string email = json["email"].asString();
    std::string password = json["password"].asString();

    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database connection failed");
        callback(resp);
        return;
    }
    
    try {
        auto result = dbClient->execSqlSync("SELECT id, password_hash, role FROM users WHERE email = $1", email);
        if (result.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Invalid credentials");
            callback(resp);
            return;
        }

        std::string storedHash = result[0]["password_hash"].as<std::string>();
        std::string userId = result[0]["id"].as<std::string>();
        std::string role = result[0]["role"].as<std::string>();

        if (utils::crypto::verifyPassword(password, storedHash)) {
            std::string token = utils::jwt_utils::generateToken(userId, role);
            
            Json::Value ret;
            ret["message"] = "Login successful";
            ret["token"] = token;
            ret["user_id"] = userId;
            ret["role"] = role;

            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Invalid credentials");
            callback(resp);
        }
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database error");
        callback(resp);
    }
}
