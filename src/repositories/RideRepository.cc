#include "RideRepository.h"
#include <drogon/drogon.h>

namespace repositories {

std::string RideRepository::createRide(const std::string& passengerId, const std::string& pickup, const std::string& destination, double distance, int estTime, double estFare) {
    auto dbClient = drogon::app().getDbClient();
    auto result = dbClient->execSqlSync(
        "INSERT INTO ride (passenger_id, pickup, destination, distance_km, estimated_time, estimated_fare, ride_status) "
        "VALUES ($1, $2, $3, $4, $5, $6, 'REQUESTED') RETURNING ride_id",
        passengerId, pickup, destination, distance, estTime, estFare
    );
    if (result.empty()) throw std::runtime_error("Failed to create ride");
    return result[0]["ride_id"].as<std::string>();
}

bool RideRepository::acceptRide(const std::string& rideId, const std::string& driverId) {
    auto dbClient = drogon::app().getDbClient();
    auto result = dbClient->execSqlSync(
        "UPDATE ride SET driver_id = $1, ride_status = 'MATCHED', accepted_at = CURRENT_TIMESTAMP "
        "WHERE ride_id = $2 AND ride_status = 'REQUESTED' RETURNING ride_id",
        driverId, rideId
    );
    return !result.empty(); 
}

bool RideRepository::updateRideStatus(const std::string& rideId, const std::string& status) {
    auto dbClient = drogon::app().getDbClient();
    std::string timestampField = "updated_at";
    if (status == "ONGOING") timestampField = "started_at";
    else if (status == "COMPLETED") timestampField = "completed_at";

    auto result = dbClient->execSqlSync(
        "UPDATE ride SET ride_status = CAST($1 AS ride_state), " + timestampField + " = CURRENT_TIMESTAMP WHERE ride_id = $2 RETURNING ride_id",
        status, rideId
    );
    return !result.empty();
}

Json::Value RideRepository::getRide(const std::string& rideId) {
    auto dbClient = drogon::app().getDbClient();
    auto result = dbClient->execSqlSync("SELECT * FROM ride WHERE ride_id = $1", rideId);
    if (result.empty()) return Json::Value();
    
    Json::Value ride;
    ride["ride_id"] = result[0]["ride_id"].as<std::string>();
    ride["passenger_id"] = result[0]["passenger_id"].as<std::string>();
    ride["driver_id"] = result[0]["driver_id"].isNull() ? "" : result[0]["driver_id"].as<std::string>();
    ride["ride_status"] = result[0]["ride_status"].as<std::string>();
    ride["pickup"] = result[0]["pickup"].as<std::string>();
    ride["destination"] = result[0]["destination"].as<std::string>();
    return ride;
}

// Conditional update: ride must belong to driverId AND be in priorStatus.
bool RideRepository::updateRideStatusByDriver(const std::string& rideId,
                                               const std::string& driverId,
                                               const std::string& priorStatus,
                                               const std::string& newStatus) {
    auto dbClient = drogon::app().getDbClient();
    std::string timestampField = "updated_at";
    if (newStatus == "ONGOING")   timestampField = "started_at";
    else if (newStatus == "COMPLETED") timestampField = "completed_at";

    auto result = dbClient->execSqlSync(
        "UPDATE ride SET ride_status = CAST($1 AS ride_state), " + timestampField + " = CURRENT_TIMESTAMP "
        "WHERE ride_id = $2 AND driver_id = $3 AND ride_status = CAST($4 AS ride_state) RETURNING ride_id",
        newStatus, rideId, driverId, priorStatus
    );
    return !result.empty();
}

// Cancel: driver can cancel from REQUESTED or MATCHED; passenger can cancel from REQUESTED or MATCHED.
// Exactly one of actorDriverId / actorPassengerId will be non-empty.
bool RideRepository::cancelRideByActor(const std::string& rideId,
                                        const std::string& actorDriverId,
                                        const std::string& actorPassengerId) {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::Result result;
    if (!actorDriverId.empty()) {
        result = dbClient->execSqlSync(
            "UPDATE ride SET ride_status = 'CANCELLED', updated_at = CURRENT_TIMESTAMP "
            "WHERE ride_id = $1 AND driver_id = $2 AND ride_status IN ('REQUESTED','MATCHED') RETURNING ride_id",
            rideId, actorDriverId
        );
    } else {
        result = dbClient->execSqlSync(
            "UPDATE ride SET ride_status = 'CANCELLED', updated_at = CURRENT_TIMESTAMP "
            "WHERE ride_id = $1 AND passenger_id = $2 AND ride_status IN ('REQUESTED','MATCHED') RETURNING ride_id",
            rideId, actorPassengerId
        );
    }
    return !result.empty();
}

Json::Value RideRepository::getActiveRideForUser(const std::string& userId, const std::string& role) {
    auto dbClient = drogon::app().getDbClient();
    drogon::orm::Result result;
    if (role == "PASSENGER") {
        result = dbClient->execSqlSync(
            "SELECT r.* FROM ride r JOIN passenger p ON r.passenger_id = p.passenger_id "
            "WHERE p.user_id = $1 AND r.ride_status NOT IN ('COMPLETED', 'CANCELLED') LIMIT 1", userId
        );
    } else {
        result = dbClient->execSqlSync(
            "SELECT r.* FROM ride r JOIN driver d ON r.driver_id = d.driver_id "
            "WHERE d.user_id = $1 AND r.ride_status NOT IN ('COMPLETED', 'CANCELLED') LIMIT 1", userId
        );
    }
    if (result.empty()) return Json::Value();

    Json::Value ride;
    ride["ride_id"]     = result[0]["ride_id"].as<std::string>();
    ride["passenger_id"] = result[0]["passenger_id"].as<std::string>();
    ride["driver_id"]   = result[0]["driver_id"].isNull() ? "" : result[0]["driver_id"].as<std::string>();
    ride["ride_status"] = result[0]["ride_status"].as<std::string>();
    ride["pickup"]      = result[0]["pickup"].as<std::string>();
    ride["destination"] = result[0]["destination"].as<std::string>();
    return ride;
}

} // namespace repositories

