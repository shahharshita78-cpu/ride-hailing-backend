#include "RedisUtils.h"
#include <cstdlib>
#include <string>
#include <drogon/drogon.h>

namespace utils {
namespace redis {

// Thread-safe lazy singleton Redis client.
// sw::redis::Redis is movable but not copyable, so we hold it in unique_ptr.
sw::redis::Redis& getRedisClient() {
    static sw::redis::Redis* redisPtr = nullptr;
    static std::once_flag initFlag;

    std::call_once(initFlag, []() {
        const char* host = std::getenv("REDIS_HOST");
        const char* port = std::getenv("REDIS_PORT");

        std::string url = "tcp://";
        url += (host ? host : "redis");
        url += ":";
        url += (port ? port : "6379");

        LOG_INFO << "Initializing Redis client: " << url;

        sw::redis::ConnectionOptions opts;
        opts.host = (host ? host : "redis");
        opts.port = port ? std::stoi(port) : 6379;
        opts.socket_timeout = std::chrono::milliseconds(500);
        opts.connect_timeout = std::chrono::milliseconds(2000);

        sw::redis::ConnectionPoolOptions poolOpts;
        poolOpts.size = 4;

        static sw::redis::Redis redis(opts, poolOpts);
        redisPtr = &redis;
    });

    return *redisPtr;
}

void updateDriverLocation(const std::string& driverId,
                          double longitude, double latitude) {
    try {
        // geoadd key longitude latitude member
        getRedisClient().geoadd(
            "driver_locations",
            std::make_tuple(driverId, longitude, latitude));
    } catch (const std::exception& e) {
        LOG_ERROR << "Redis updateDriverLocation error: " << e.what();
    }
}

void removeDriverLocation(const std::string& driverId) {
    try {
        getRedisClient().zrem("driver_locations", driverId);
    } catch (const std::exception& e) {
        LOG_ERROR << "Redis removeDriverLocation error: " << e.what();
    }
}

std::vector<std::string> getNearbyDrivers(double longitude,
                                          double latitude,
                                          double radiusKm) {
    std::vector<std::string> drivers;
    try {
        // georadius is deprecated in Redis 6.2+ but still available.
        // redis-plus-plus wraps it with georadius("key", {lon,lat}, radius, unit, output_iter)
        getRedisClient().georadius(
            "driver_locations",
            std::make_pair(longitude, latitude),
            radiusKm,
            sw::redis::GeoUnit::KM,
            10,    // count
            true,  // ASC sort
            std::back_inserter(drivers));
    } catch (const std::exception& e) {
        LOG_ERROR << "Redis getNearbyDrivers error: " << e.what();
    }
    return drivers;
}

} // namespace redis
} // namespace utils
