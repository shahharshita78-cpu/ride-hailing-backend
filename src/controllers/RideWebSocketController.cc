#include "RideWebSocketController.h"
#include "../utils/JwtUtils.h"

using namespace api::v1;

std::unordered_map<std::string, std::set<WebSocketConnectionPtr>> RideWebSocketController::userConnections_;
std::mutex RideWebSocketController::mutex_;

void RideWebSocketController::handleNewMessage(const WebSocketConnectionPtr& wsConnPtr,
                                              std::string &&message,
                                              const WebSocketMessageType &type) {
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
    
    wsConnPtr->setContext(std::make_shared<std::string>(userId));
    
    std::lock_guard<std::mutex> lock(mutex_);
    userConnections_[userId].insert(wsConnPtr);
}

void RideWebSocketController::handleConnectionClosed(const WebSocketConnectionPtr& wsConnPtr) {
    if (wsConnPtr->hasContext()) {
        auto userIdPtr = wsConnPtr->getContext<std::string>();
        std::lock_guard<std::mutex> lock(mutex_);
        userConnections_[*userIdPtr].erase(wsConnPtr);
        if (userConnections_[*userIdPtr].empty()) {
            userConnections_.erase(*userIdPtr);
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
