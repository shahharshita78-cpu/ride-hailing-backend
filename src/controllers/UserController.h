#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
namespace v1 {

class UserController : public drogon::HttpController<UserController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(UserController::signup, "/auth/signup", Post);
    ADD_METHOD_TO(UserController::login, "/auth/login", Post);
    METHOD_LIST_END

    void signup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void login(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};

} // namespace v1
} // namespace api
