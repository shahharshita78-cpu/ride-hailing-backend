#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
namespace drivers {

class DriverController : public drogon::HttpController<DriverController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(DriverController::getDriver, "/api/drivers/{id}", drogon::Get);
    ADD_METHOD_TO(DriverController::getVehicle, "/api/drivers/{id}/vehicle", drogon::Get);
    METHOD_LIST_END

    void getDriver(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
    void getVehicle(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
};

}
}
