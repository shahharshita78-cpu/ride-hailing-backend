#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
namespace v1 {

class RideController : public drogon::HttpController<RideController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RideController::requestRide, "/rides/request", Post, "JwtFilter");
    ADD_METHOD_TO(RideController::getRide, "/rides/{id}", Get, "JwtFilter");
    ADD_METHOD_TO(RideController::cancelRide, "/rides/{id}/cancel", Post, "JwtFilter");
    METHOD_LIST_END

    void requestRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
    void cancelRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
};

}
}
