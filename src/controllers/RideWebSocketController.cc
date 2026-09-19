#include "RideWebSocketController.h"
#include "../utils/JwtUtils.h"
#include "../utils/RedisUtils.h"
#include <drogon/orm/DbClient.h>
#include <json/json.h>

using namespace api::rides; // Changed namespace

std::unordered_map<std::string, std::set<WebSocketConnectionPtr>> RideWebSocketController::userConnections_;
std::mutex RideWebSocketController::mutex_;

void RideWebSocketController::handleNewMessage(const WebSocketConnectionPtr& wsConnPtr,
                                              std::string &&message,
                                              const WebSocketMessageType &type) {
    if (wsConnPtr->hasContext()) {
        auto context = wsConnPtr->getContext<std::pair<std::string, std::string>>(); // <userId, driverId>
        std::string userId = context->first;
        std::string driverId = context->second;

        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(message, root) && root.isMember("action") && root["action"].asString() == "location") {
                if (!driverId.empty()) {
                    double lat = root["lat"].asDouble();
                    double lon = root["lon"].asDouble();
                    utils::redis::updateDriverLocation(driverId, lon, lat);
                    wsConnPtr->send("{\"status\":\"Location updated\"}");

                    // Notify passenger if there is an active ride (async to avoid blocking event loop)
                    auto dbClient = drogon::app().getDbClient();
                    dbClient->execSqlAsync(
                        "SELECT p.user_id FROM passenger p JOIN ride r ON p.passenger_id = r.passenger_id "
                        "WHERE r.driver_id = $1 AND r.ride_status IN ('MATCHED', 'DRIVER_ARRIVING', 'ONGOING') LIMIT 1",
                        [lat, lon](const drogon::orm::Result& pRes) {
                            if (!pRes.empty()) {
                                std::string passUserId = pRes[0]["user_id"].as<std::string>();
                                Json::Value locationMsg;
                                locationMsg["event"] = "location_update";
                                locationMsg["lat"] = lat;
                                locationMsg["lon"] = lon;
                                Json::FastWriter writer;
                                RideWebSocketController::notifyUser(passUserId, writer.write(locationMsg));
                            }
                        },
                        [](const drogon::orm::DrogonDbException& e) {
                            LOG_ERROR << "DB error notifying passenger of location: " << e.base().what();
                        },
                        driverId
                    );
                } else {
                    wsConnPtr->send("{\"error\":\"Only drivers can update location\"}");
                }
                return;
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "WS message parse error: " << e.what();
        }
    }
    wsConnPtr->send("Received: " + message);
}

void RideWebSocketController::handleNewConnection(const HttpRequestPtr &req,
                                                 const WebSocketConnectionPtr& wsConnPtr) {
    std::string token = req->getParameter("token");
    std::string userId, role;
    
    if (!utils::jwt_utils::verifyToken(token, userId, role)) {
        wsConnPtr->forceClose();
        return;
    }
    
    if (role == "DRIVER") {
        auto dbClient = drogon::app().getDbClient();
        dbClient->execSqlAsync(
            "SELECT driver_id FROM driver WHERE user_id = $1",
            [wsConnPtr, userId](const drogon::orm::Result& dRes) {
                std::string driverId = "";
                if (!dRes.empty()) {
                    driverId = dRes[0]["driver_id"].as<std::string>();
                }
                wsConnPtr->setContext(std::make_shared<std::pair<std::string, std::string>>(userId, driverId));
                std::lock_guard<std::mutex> lock(mutex_);
                userConnections_[userId].insert(wsConnPtr);
            },
            [wsConnPtr](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << "DB error on WS connect: " << e.base().what();
                wsConnPtr->forceClose();
            },
            userId
        );
    } else {
        wsConnPtr->setContext(std::make_shared<std::pair<std::string, std::string>>(userId, ""));
        std::lock_guard<std::mutex> lock(mutex_);
        userConnections_[userId].insert(wsConnPtr);
    }
}

void RideWebSocketController::handleConnectionClosed(const WebSocketConnectionPtr& wsConnPtr) {
    if (wsConnPtr->hasContext()) {
        auto context = wsConnPtr->getContext<std::pair<std::string, std::string>>();
        std::string userId = context->first;
        std::lock_guard<std::mutex> lock(mutex_);
        userConnections_[userId].erase(wsConnPtr);
        if (userConnections_[userId].empty()) {
            userConnections_.erase(userId);
        }
    }
}

void RideWebSocketController::notifyUser(const std::string& userId, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (userConnections_.find(userId) != userConnections_.end()) {
        for (auto& conn : userConnections_[userId]) {
            conn->send(message);
        }
    }
}

void RideWebSocketController::notifyPassengerByRideId(const std::string& rideId, const Json::Value& message) {
    try {
        auto dbClient = drogon::app().getDbClient();
        Json::FastWriter writer;
        std::string msgStr = writer.write(message);
        dbClient->execSqlAsync(
            "SELECT p.user_id FROM passenger p JOIN ride r ON p.passenger_id = r.passenger_id WHERE r.ride_id = $1",
            [msgStr](const drogon::orm::Result& pRes) {
                if (!pRes.empty()) {
                    std::string passUserId = pRes[0]["user_id"].as<std::string>();
                    RideWebSocketController::notifyUser(passUserId, msgStr);
                }
            },
            [](const drogon::orm::DrogonDbException& e) {
                LOG_ERROR << "Failed to notify passenger: " << e.base().what();
            },
            rideId
        );
    } catch (const std::exception& e) {
        LOG_ERROR << "notifyPassengerByRideId error: " << e.what();
    }
}

void RideWebSocketController::notifyAllDrivers(const Json::Value& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    Json::FastWriter writer;
    std::string msgStr = writer.write(message);
    for (const auto& [userId, conns] : userConnections_) {
        if (conns.empty()) continue;
        auto context = (*conns.begin())->getContext<std::pair<std::string, std::string>>();
        if (context && !context->second.empty()) {
            for (const auto& conn : conns) {
                conn->send(msgStr);
            }
        }
    }
}
