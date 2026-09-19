#include "DriverController.h"
#include <drogon/orm/DbClient.h>

using namespace api::drivers;

void DriverController::getDriver(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto dbClient = drogon::app().getDbClient();
    try {
        auto result = dbClient->execSqlSync("SELECT * FROM driver WHERE driver_id = $1", id);
        if (result.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }

        Json::Value driver;
        driver["driver_id"] = result[0]["driver_id"].as<std::string>();
        driver["current_status"] = result[0]["current_status"].as<std::string>();
        driver["avg_rating"] = result[0]["avg_rating"].as<double>();

        auto resp = HttpResponse::newHttpJsonResponse(driver);
        callback(resp);
    } catch (const std::exception &e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void DriverController::getVehicle(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto dbClient = drogon::app().getDbClient();
    try {
        auto result = dbClient->execSqlSync("SELECT * FROM vehicle WHERE driver_id = $1", id);
        if (result.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }

        Json::Value vehicle;
        vehicle["vehicle_id"] = result[0]["vehicle_id"].as<std::string>();
        vehicle["vehicle_number"] = result[0]["vehicle_number"].as<std::string>();
        vehicle["vehicle_type"] = result[0]["vehicle_type"].as<std::string>();

        auto resp = HttpResponse::newHttpJsonResponse(vehicle);
        callback(resp);
    } catch (const std::exception &e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}
