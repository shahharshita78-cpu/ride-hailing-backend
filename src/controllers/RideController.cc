#include "RideController.h"
#include "RideWebSocketController.h"
#include "../utils/KafkaUtils.h"
#include "../utils/RedisUtils.h"
#include <drogon/orm/DbClient.h>

using namespace api::v1;

void RideController::requestRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role = req->getAttributes()->get<std::string>("role");

    if (role != "rider") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Only riders can request rides");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("pickup_lat") || !jsonPtr->isMember("pickup_lon") ||
        !jsonPtr->isMember("dropoff_lat") || !jsonPtr->isMember("dropoff_lon")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing coordinates");
        callback(resp);
        return;
    }

    double pLat = (*jsonPtr)["pickup_lat"].asDouble();
    double pLon = (*jsonPtr)["pickup_lon"].asDouble();
    double dLat = (*jsonPtr)["dropoff_lat"].asDouble();
    double dLon = (*jsonPtr)["dropoff_lon"].asDouble();

    auto nearbyDrivers = utils::redis::getNearbyDrivers(pLon, pLat, 10.0);
    if (nearbyDrivers.empty()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("No nearby drivers available");
        callback(resp);
        return;
    }

    std::string matchedDriverId = nearbyDrivers[0];

    auto dbClient = drogon::app().getDbClient();
    if (!dbClient) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database connection failed");
        callback(resp);
        return;
    }

    try {
        auto result = dbClient->execSqlSync(
            "INSERT INTO rides (rider_id, driver_id, pickup_lat, pickup_lon, dropoff_lat, dropoff_lon, status) VALUES ($1, $2, $3, $4, $5, $6, 'REQUESTED') RETURNING id",
            userId, matchedDriverId, pLat, pLon, dLat, dLon
        );

        if (!result.empty()) {
            std::string rideId = result[0]["id"].as<std::string>();
            
            utils::kafka::produceEvent("ride_events", rideId, rideId + ":REQUESTED");
            utils::kafka::produceEvent("ride_events", rideId, rideId + ":MATCHED");

            Json::Value driverMsg;
            driverMsg["type"] = "ride_matched";
            driverMsg["ride_id"] = rideId;
            driverMsg["pickup_lat"] = pLat;
            driverMsg["pickup_lon"] = pLon;
            RideWebSocketController::notifyUser(matchedDriverId, driverMsg.toStyledString());

            Json::Value ret;
            ret["ride_id"] = rideId;
            ret["driver_id"] = matchedDriverId;
            ret["status"] = "MATCHED";

            auto resp = HttpResponse::newHttpJsonResponse(ret);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            resp->setBody("Failed to create ride");
            callback(resp);
        }
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Database error");
        callback(resp);
    }
}

void RideController::getRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto dbClient = drogon::app().getDbClient();
    try {
        auto result = dbClient->execSqlSync("SELECT * FROM rides WHERE id = $1", id);
        if (result.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            resp->setBody("Ride not found");
            callback(resp);
            return;
        }

        Json::Value ret;
        ret["id"] = result[0]["id"].as<std::string>();
        ret["status"] = result[0]["status"].as<std::string>();
        ret["driver_id"] = result[0]["driver_id"].as<std::string>();

        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const std::exception &e) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void RideController::cancelRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    utils::kafka::produceEvent("ride_events", id, id + ":CANCELLED");
    
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    resp->setBody("Ride cancelled");
    callback(resp);
}
