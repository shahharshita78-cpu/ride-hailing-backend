#pragma once
#include <string>
#include <json/json.h>

namespace repositories {
class RideRepository {
public:
    static std::string createRide(const std::string& passengerId, const std::string& pickup, const std::string& destination, double distance, int estTime, double estFare);
    static bool acceptRide(const std::string& rideId, const std::string& driverId);
    static bool updateRideStatus(const std::string& rideId, const std::string& status);
    static Json::Value getRide(const std::string& rideId);
    static Json::Value getActiveRideForUser(const std::string& userId, const std::string& role);
};
}
