#pragma once
#include <drogon/WebSocketController.h>
#include <unordered_map>
#include <mutex>
#include <set>

using namespace drogon;

namespace api {
namespace rides {

class RideWebSocketController : public drogon::WebSocketController<RideWebSocketController> {
public:
    void handleNewMessage(const WebSocketConnectionPtr&,
                          std::string &&,
                          const WebSocketMessageType &) override;
    void handleNewConnection(const HttpRequestPtr &,
                             const WebSocketConnectionPtr&) override;
    void handleConnectionClosed(const WebSocketConnectionPtr&) override;
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/api/ws");
    WS_PATH_LIST_END

    static void notifyUser(const std::string& userId, const std::string& message);
    static void notifyPassengerByRideId(const std::string& rideId, const Json::Value& message);

private:
    static std::unordered_map<std::string, std::set<WebSocketConnectionPtr>> userConnections_;
    static std::mutex mutex_;
};

}
}
