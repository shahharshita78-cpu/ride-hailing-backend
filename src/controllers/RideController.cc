#include "RideController.h"
#include "../controllers/RideWebSocketController.h"
#include "../utils/KafkaUtils.h"
#include "../repositories/RideRepository.h"
#include <drogon/orm/DbClient.h>

using namespace api::rides;

void RideController::requestRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role = req->getAttributes()->get<std::string>("role");

    if (role != "PASSENGER") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Only passengers can request rides");
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    auto pRes = dbClient->execSqlSync("SELECT passenger_id FROM passenger WHERE user_id = $1", userId);
    if (pRes.empty()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
    }
    std::string passengerId = pRes[0]["passenger_id"].as<std::string>();

    std::string pickup = (*jsonPtr)["pickup"].asString();
    std::string destination = (*jsonPtr)["destination"].asString();
    double distance = jsonPtr->isMember("distance_km") ? (*jsonPtr)["distance_km"].asDouble() : 0.0;
    int estTime = jsonPtr->isMember("estimated_time") ? (*jsonPtr)["estimated_time"].asInt() : 0;
    double estFare = jsonPtr->isMember("estimated_fare") ? (*jsonPtr)["estimated_fare"].asDouble() : 0.0;

    try {
        std::string rideId = repositories::RideRepository::createRide(passengerId, pickup, destination, distance, estTime, estFare);
        
        utils::kafka::produceEvent("ride_events", rideId, "RideRequested");

        Json::Value ret;
        ret["ride_id"] = rideId;
        ret["status"] = "REQUESTED";

        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void RideController::acceptRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role = req->getAttributes()->get<std::string>("role");

    if (role != "DRIVER") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    auto dRes = dbClient->execSqlSync("SELECT driver_id FROM driver WHERE user_id = $1", userId);
    if (dRes.empty()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
        return;
    }
    std::string driverId = dRes[0]["driver_id"].as<std::string>();

    bool success = repositories::RideRepository::acceptRide(id, driverId);
    if (success) {
        utils::kafka::produceEvent("ride_events", id, "RideAccepted");
        
        Json::Value msg;
        msg["event"] = "status_update";
        msg["status"] = "MATCHED";
        msg["ride_id"] = id;
        RideWebSocketController::notifyPassengerByRideId(id, msg);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setBody("Ride accepted successfully");
        callback(resp);
    } else {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k409Conflict);
        resp->setBody("Ride no longer available");
        callback(resp);
    }
}

void RideController::startRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    bool success = repositories::RideRepository::updateRideStatus(id, "ONGOING");
    if (success) {
        utils::kafka::produceEvent("ride_events", id, "RideStarted");
        
        Json::Value msg;
        msg["event"] = "status_update";
        msg["status"] = "ONGOING";
        msg["ride_id"] = id;
        RideWebSocketController::notifyPassengerByRideId(id, msg);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        callback(resp);
    } else {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void RideController::completeRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    bool success = repositories::RideRepository::updateRideStatus(id, "COMPLETED");
    if (success) {
        utils::kafka::produceEvent("ride_events", id, "RideCompleted");

        Json::Value msg;
        msg["event"] = "status_update";
        msg["status"] = "COMPLETED";
        msg["ride_id"] = id;
        RideWebSocketController::notifyPassengerByRideId(id, msg);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        callback(resp);
    } else {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void RideController::cancelRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    bool success = repositories::RideRepository::updateRideStatus(id, "CANCELLED");
    if (success) {
        utils::kafka::produceEvent("ride_events", id, "RideCancelled");

        Json::Value msg;
        msg["event"] = "status_update";
        msg["status"] = "CANCELLED";
        msg["ride_id"] = id;
        RideWebSocketController::notifyPassengerByRideId(id, msg);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        callback(resp);
    } else {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void RideController::getRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    Json::Value ride = repositories::RideRepository::getRide(id);
    if (ride.isNull()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        callback(resp);
        return;
    }
    auto resp = HttpResponse::newHttpJsonResponse(ride);
    callback(resp);
}

void RideController::getHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    // Simplified for demo, returns an empty array
    Json::Value arr(Json::arrayValue);
    auto resp = HttpResponse::newHttpJsonResponse(arr);
    callback(resp);
}
