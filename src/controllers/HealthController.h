#pragma once
#include <drogon/HttpController.h>

namespace api {
namespace health {

class HealthController : public drogon::HttpController<HealthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(HealthController::check, "/health", Get);
    METHOD_LIST_END

    void check(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

} // namespace health
} // namespace api
