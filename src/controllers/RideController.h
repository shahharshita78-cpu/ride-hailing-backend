#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
namespace rides {

class RideController : public drogon::HttpController<RideController> {
public:
    METHOD_LIST_BEGIN
    // IMPORTANT: Static concrete paths MUST be registered before parameterized {id} paths.
    // Drogon is first-match-wins; /api/rides/{id} would otherwise shadow /api/rides/history.
    ADD_METHOD_TO(RideController::requestRide,  "/api/rides",          Post, "JwtFilter");
    ADD_METHOD_TO(RideController::getHistory,   "/api/rides/history",  Get,  "JwtFilter");
    ADD_METHOD_TO(RideController::getActiveRide,"/api/rides/active",   Get,  "JwtFilter");
    ADD_METHOD_TO(RideController::getRide,      "/api/rides/{id}",     Get,  "JwtFilter");
    ADD_METHOD_TO(RideController::acceptRide,   "/api/rides/{id}/accept",   Post, "JwtFilter");
    ADD_METHOD_TO(RideController::startRide,    "/api/rides/{id}/start",    Post, "JwtFilter");
    ADD_METHOD_TO(RideController::completeRide, "/api/rides/{id}/complete", Post, "JwtFilter");
    ADD_METHOD_TO(RideController::cancelRide,   "/api/rides/{id}/cancel",   Post, "JwtFilter");
    METHOD_LIST_END

    void requestRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
    void getHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getActiveRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    
    void acceptRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
    void startRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
    void completeRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
    void cancelRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
};

}
}
