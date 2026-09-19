#include "RedisUtils.h"
#include <cstdlib>

namespace utils {
namespace redis {

sw::redis::Redis& getRedisClient() {
    static sw::redis::Redis redis([]() {
        std::string host = std::getenv("REDIS_HOST") ? std::getenv("REDIS_HOST") : "127.0.0.1";
        std::string port = std::getenv("REDIS_PORT") ? std::getenv("REDIS_PORT") : "6379";
        return sw::redis::Redis("tcp://" + host + ":" + port);
    }());
    return redis;
}

void updateDriverLocation(const std::string& driverId, double longitude, double latitude) {
    getRedisClient().geoadd("driver_locations", std::make_tuple(driverId, longitude, latitude));
}

void removeDriverLocation(const std::string& driverId) {
    getRedisClient().zrem("driver_locations", driverId);
}

std::vector<std::string> getNearbyDrivers(double longitude, double latitude, double radiusKm) {
    std::vector<std::string> drivers;
    getRedisClient().georadius("driver_locations", std::make_pair(longitude, latitude), 
                               radiusKm, sw::redis::GeoUnit::KM, 10, true, std::back_inserter(drivers));
    return drivers;
}

}
}
