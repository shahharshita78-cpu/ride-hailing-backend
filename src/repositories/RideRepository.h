#pragma once
#include <string>
#include <json/json.h>

namespace repositories {
class RideRepository {
public:
    static std::string createRide(const std::string& passengerId, const std::string& pickup, const std::string& destination, double distance, int estTime, double estFare);
    static bool acceptRide(const std::string& rideId, const std::string& driverId);
    static bool updateRideStatus(const std::string& rideId, const std::string& status);
    // Requires the ride to belong to driverId AND be in priorStatus. Returns false → 403 or 409.
    static bool updateRideStatusByDriver(const std::string& rideId, const std::string& driverId, const std::string& priorStatus, const std::string& newStatus);
    // Cancel: allowed by driver (REQUESTED or MATCHED) or passenger (REQUESTED or MATCHED).
    static bool cancelRideByActor(const std::string& rideId, const std::string& actorDriverId, const std::string& actorPassengerId);
    static Json::Value getRide(const std::string& rideId);
    static Json::Value getActiveRideForUser(const std::string& userId, const std::string& role);
};
}
