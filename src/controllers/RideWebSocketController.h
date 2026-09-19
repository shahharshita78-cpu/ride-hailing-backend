#pragma once
#include <drogon/WebSocketController.h>
#include <unordered_map>
#include <mutex>
#include <set>

using namespace drogon;

namespace api {
namespace v1 {

class RideWebSocketController : public drogon::WebSocketController<RideWebSocketController> {
public:
    void handleNewMessage(const WebSocketConnectionPtr&,
                          std::string &&,
                          const WebSocketMessageType &) override;
    void handleNewConnection(const HttpRequestPtr &,
                             const WebSocketConnectionPtr&) override;
    void handleConnectionClosed(const WebSocketConnectionPtr&) override;
    WS_PATH_LIST_BEGIN
    WS_PATH_ADD("/ws/rides");
    WS_PATH_LIST_END

    static void notifyUser(const std::string& userId, const std::string& message);

private:
    static std::unordered_map<std::string, std::set<WebSocketConnectionPtr>> userConnections_;
    static std::mutex mutex_;
};

}
}
