#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
namespace v1 {

class DriverController : public drogon::HttpController<DriverController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(DriverController::registerDriver, "/drivers/register", Post, "JwtFilter");
    ADD_METHOD_TO(DriverController::updateStatus, "/drivers/status", Patch, "JwtFilter");
    ADD_METHOD_TO(DriverController::updateLocation, "/drivers/location", Patch, "JwtFilter");
    ADD_METHOD_TO(DriverController::getNearby, "/drivers/nearby", Get);
    METHOD_LIST_END

    void registerDriver(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void updateStatus(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void updateLocation(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getNearby(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};

}
}
