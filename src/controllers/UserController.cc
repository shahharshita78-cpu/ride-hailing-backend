#include "UserController.h"
#include "../utils/CryptoUtils.h"
#include "../utils/JwtUtils.h"
#include "../repositories/UserRepository.h"
#include <drogon/orm/DbClient.h>

using namespace api::auth;

void UserController::registerUser(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    LOG_INFO << "UserController: registerUser HTTP handler called!";
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing JSON body");
        callback(resp);
        return;
    }

    auto& json = *jsonPtr;
    if (!json.isMember("email") || !json.isMember("password") || !json.isMember("role") || !json.isMember("name") || !json.isMember("phone")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing required fields");
        callback(resp);
        return;
    }

    std::string email = json["email"].asString();
    std::string password = json["password"].asString();
    std::string role = json["role"].asString();
    std::string name = json["name"].asString();
    std::string phone = json["phone"].asString();

    if (role != "PASSENGER" && role != "DRIVER") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Role must be PASSENGER or DRIVER");
        callback(resp);
        return;
    }

    std::string salt = ::utils::crypto::generateSalt();
    std::string passwordHash = ::utils::crypto::hashPassword(password, salt);

    LOG_INFO << "UserController: Calling UserRepository::createUser for email " << email;
    repositories::UserRepository::createUser(
        name, email, phone, passwordHash, role,
        [callback, name, email, role](const std::string& userId) {
            LOG_INFO << "UserController: createUser SUCCESS, generating token";
            std::string token = ::utils::jwt_utils::generateToken(userId, role);

            Json::Value userObj;
            userObj["user_id"] = userId;
            userObj["name"] = name;
            userObj["email"] = email;
            userObj["role"] = role;

            Json::Value ret;
            ret["message"] = "Registration successful";
            ret["token"] = token;
            ret["user"] = userObj;

            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
        },
        [callback](const std::exception& e) {
            LOG_ERROR << e.what();
            Json::Value ret;
            ret["message"] = std::string("DB Error: ") + e.what();
            auto resp = HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(k500InternalServerError);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            callback(resp);
        }
    );
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

    repositories::UserRepository::getUserByEmail(
        email,
        [callback, password, email](const Json::Value& user) {
            if (user.isNull()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Invalid credentials");
                callback(resp);
                return;
            }

            std::string storedHash = user["password_hash"].asString();
            std::string userId = user["user_id"].asString();
            std::string role = user["role"].asString();

            if (::utils::crypto::verifyPassword(password, storedHash)) {
                std::string token = ::utils::jwt_utils::generateToken(userId, role);
                
                Json::Value userObj;
                userObj["user_id"] = userId;
                userObj["name"] = user["name"].asString();
                userObj["email"] = email;
                userObj["role"] = role;
                
                if (user.isMember("passenger_id")) userObj["passenger_id"] = user["passenger_id"];
                if (user.isMember("driver_id")) userObj["driver_id"] = user["driver_id"];

                Json::Value ret;
                ret["message"] = "Login successful";
                ret["token"] = token;
                ret["user"] = userObj;

                auto resp = HttpResponse::newHttpJsonResponse(ret);
                callback(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("Invalid credentials");
                callback(resp);
            }
        },
        [callback](const std::exception& e) {
            LOG_ERROR << e.what();
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->setBody("Login failed");
            callback(resp);
        }
    );
}
