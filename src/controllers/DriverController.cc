#include "DriverController.h"
#include "../utils/RedisUtils.h"
#include <drogon/orm/DbClient.h>

using namespace api::v1;

void DriverController::registerDriver(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role = req->getAttributes()->get<std::string>("role");
    
    if (role != "driver") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Only drivers can register vehicle details");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("vehicle_details")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing vehicle_details");
        callback(resp);
        return;
    }

    std::string vehicleDetails = (*jsonPtr)["vehicle_details"].asString();
    
    auto dbClient = drogon::app().getDbClient();
    try {
        dbClient->execSqlSync("INSERT INTO drivers (user_id, vehicle_details) VALUES ($1, $2) ON CONFLICT (user_id) DO UPDATE SET vehicle_details = EXCLUDED.vehicle_details", userId, vehicleDetails);
        
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setBody("Driver registered");
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database error");
        callback(resp);
    }
}

void DriverController::updateStatus(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("is_online")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing is_online");
        callback(resp);
        return;
    }

    bool isOnline = (*jsonPtr)["is_online"].asBool();

    auto dbClient = drogon::app().getDbClient();
    try {
        dbClient->execSqlSync("UPDATE drivers SET is_online = $1 WHERE user_id = $2", isOnline, userId);
        
        if (!isOnline) {
            utils::redis::removeDriverLocation(userId);
        }

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setBody("Status updated");
        callback(resp);
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << e.base().what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database error");
        callback(resp);
    }
}

void DriverController::updateLocation(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("latitude") || !jsonPtr->isMember("longitude")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing latitude or longitude");
        callback(resp);
        return;
    }

    double lat = (*jsonPtr)["latitude"].asDouble();
    double lon = (*jsonPtr)["longitude"].asDouble();

    try {
        utils::redis::updateDriverLocation(userId, lon, lat);
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setBody("Location updated");
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << "Redis error: " << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Redis error");
        callback(resp);
    }
}

void DriverController::getNearby(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto latStr = req->getParameter("lat");
    auto lonStr = req->getParameter("lon");
    auto radiusStr = req->getParameter("radius");
    
    if (latStr.empty() || lonStr.empty()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing lat or lon");
        callback(resp);
        return;
    }

    double lat = std::stod(latStr);
    double lon = std::stod(lonStr);
    double radius = radiusStr.empty() ? 5.0 : std::stod(radiusStr);

    try {
        auto drivers = utils::redis::getNearbyDrivers(lon, lat, radius);
        Json::Value ret(Json::arrayValue);
        for (const auto& d : drivers) {
            ret.append(d);
        }

        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << "Redis error: " << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Redis error");
        callback(resp);
    }
}
