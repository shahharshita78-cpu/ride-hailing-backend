#include "RideController.h"
#include "../controllers/RideWebSocketController.h"
#include "../utils/KafkaUtils.h"
#include "../repositories/RideRepository.h"
#include <drogon/orm/DbClient.h>

using namespace api::rides;

void RideController::requestRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role   = req->getAttributes()->get<std::string>("role");

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

    std::string pickup      = (*jsonPtr)["pickup"].asString();
    std::string destination = (*jsonPtr)["destination"].asString();
    double distance = jsonPtr->isMember("distance_km")       ? (*jsonPtr)["distance_km"].asDouble()     : 0.0;
    int    estTime  = jsonPtr->isMember("estimated_time")     ? (*jsonPtr)["estimated_time"].asInt()     : 0;
    double estFare  = jsonPtr->isMember("estimated_fare")     ? (*jsonPtr)["estimated_fare"].asDouble()  : 0.0;

    try {
        std::string rideId = repositories::RideRepository::createRide(passengerId, pickup, destination, distance, estTime, estFare);

        // FIX 4: Kafka failure must not abort the ride or the WS notification.
        try {
            utils::kafka::produceEvent("ride_events", rideId, "RideRequested");
        } catch (const std::exception& kafkaEx) {
            LOG_ERROR << "requestRide: Kafka produce failed (ride still created): " << kafkaEx.what();
        }

        Json::Value ret;
        ret["ride_id"] = rideId;
        ret["status"]  = "REQUESTED";

        Json::Value driverMsg;
        driverMsg["event"]          = "ride_request";
        driverMsg["ride_id"]        = rideId;
        driverMsg["pickup"]         = pickup;
        driverMsg["destination"]    = destination;
        driverMsg["estimated_fare"] = estFare;
        RideWebSocketController::notifyAllDrivers(driverMsg);

        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << "requestRide: " << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void RideController::acceptRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role   = req->getAttributes()->get<std::string>("role");

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
        try {
            utils::kafka::produceEvent("ride_events", id, "RideAccepted");
        } catch (const std::exception& kafkaEx) {
            LOG_ERROR << "acceptRide: Kafka produce failed: " << kafkaEx.what();
        }

        Json::Value msg;
        msg["event"]   = "status_update";
        msg["status"]  = "MATCHED";
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

// FIX 3: startRide — require authenticated driver, prior status must be MATCHED.
void RideController::startRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role   = req->getAttributes()->get<std::string>("role");

    if (role != "DRIVER") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Only the driver can start a ride");
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

    // Conditional UPDATE: ride must belong to this driver AND be in MATCHED state.
    bool success = repositories::RideRepository::updateRideStatusByDriver(id, driverId, "MATCHED", "ONGOING");
    if (success) {
        try {
            utils::kafka::produceEvent("ride_events", id, "RideStarted");
        } catch (const std::exception& kafkaEx) {
            LOG_ERROR << "startRide: Kafka produce failed: " << kafkaEx.what();
        }

        Json::Value msg;
        msg["event"]   = "status_update";
        msg["status"]  = "ONGOING";
        msg["ride_id"] = id;
        RideWebSocketController::notifyPassengerByRideId(id, msg);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        callback(resp);
    } else {
        // Either the ride does not belong to this driver (403) or it's not in MATCHED state (409).
        // We distinguish by checking ownership separately.
        auto check = dbClient->execSqlSync(
            "SELECT ride_id FROM ride WHERE ride_id = $1 AND driver_id = $2", id, driverId);
        if (check.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k403Forbidden);
            resp->setBody("Not your ride");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k409Conflict);
            resp->setBody("Ride is not in MATCHED state");
            callback(resp);
        }
    }
}

// FIX 3: completeRide — require authenticated driver, prior status must be ONGOING.
void RideController::completeRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role   = req->getAttributes()->get<std::string>("role");

    if (role != "DRIVER") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Only the driver can complete a ride");
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

    // Conditional UPDATE: ride must belong to this driver AND be in ONGOING state.
    bool success = repositories::RideRepository::updateRideStatusByDriver(id, driverId, "ONGOING", "COMPLETED");
    if (success) {
        try {
            utils::kafka::produceEvent("ride_events", id, "RideCompleted");
        } catch (const std::exception& kafkaEx) {
            LOG_ERROR << "completeRide: Kafka produce failed: " << kafkaEx.what();
        }

        Json::Value msg;
        msg["event"]   = "status_update";
        msg["status"]  = "COMPLETED";
        msg["ride_id"] = id;
        RideWebSocketController::notifyPassengerByRideId(id, msg);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        callback(resp);
    } else {
        auto check = dbClient->execSqlSync(
            "SELECT ride_id FROM ride WHERE ride_id = $1 AND driver_id = $2", id, driverId);
        if (check.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k403Forbidden);
            resp->setBody("Not your ride");
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k409Conflict);
            resp->setBody("Ride is not in ONGOING state");
            callback(resp);
        }
    }
}

// FIX 3: cancelRide — driver (REQUESTED/MATCHED) or passenger (REQUESTED/MATCHED).
void RideController::cancelRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role   = req->getAttributes()->get<std::string>("role");

    if (role != "DRIVER" && role != "PASSENGER") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    auto dbClient = drogon::app().getDbClient();
    std::string actorDriverId;
    std::string actorPassengerId;

    if (role == "DRIVER") {
        auto dRes = dbClient->execSqlSync("SELECT driver_id FROM driver WHERE user_id = $1", userId);
        if (dRes.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        actorDriverId = dRes[0]["driver_id"].as<std::string>();
    } else {
        auto pRes = dbClient->execSqlSync("SELECT passenger_id FROM passenger WHERE user_id = $1", userId);
        if (pRes.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
            return;
        }
        actorPassengerId = pRes[0]["passenger_id"].as<std::string>();
    }

    bool success = repositories::RideRepository::cancelRideByActor(id, actorDriverId, actorPassengerId);
    if (success) {
        try {
            utils::kafka::produceEvent("ride_events", id, "RideCancelled");
        } catch (const std::exception& kafkaEx) {
            LOG_ERROR << "cancelRide: Kafka produce failed: " << kafkaEx.what();
        }

        Json::Value msg;
        msg["event"]   = "status_update";
        msg["status"]  = "CANCELLED";
        msg["ride_id"] = id;
        RideWebSocketController::notifyPassengerByRideId(id, msg);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        callback(resp);
    } else {
        // Either not their ride, or wrong status — return 409.
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k409Conflict);
        resp->setBody("Cannot cancel: ride not found, not yours, or in a non-cancellable state");
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

void RideController::getActiveRide(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto userId = req->getAttributes()->get<std::string>("user_id");
    auto role   = req->getAttributes()->get<std::string>("role");
    Json::Value ride = repositories::RideRepository::getActiveRideForUser(userId, role);
    auto resp = HttpResponse::newHttpJsonResponse(ride);
    callback(resp);
}
