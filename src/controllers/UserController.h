#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
namespace auth {

class UserController : public drogon::HttpController<UserController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserController::registerUser, "/api/auth/register", drogon::Post);
    ADD_METHOD_TO(UserController::login, "/api/auth/login", drogon::Post);
    METHOD_LIST_END

    void registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void login(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};

} 
} 
