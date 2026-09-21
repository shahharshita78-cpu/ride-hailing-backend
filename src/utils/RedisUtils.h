#pragma once
#include <sw/redis++/redis++.h>
#include <string>
#include <vector>
#include <mutex>

namespace utils {
namespace redis {

sw::redis::Redis& getRedisClient();

void updateDriverLocation(const std::string& driverId, double longitude, double latitude);
void removeDriverLocation(const std::string& driverId);
std::vector<std::string> getNearbyDrivers(double longitude, double latitude, double radiusKm);

} // namespace redis
} // namespace utils
