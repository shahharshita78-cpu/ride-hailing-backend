#include <drogon/drogon.h>
#include "utils/KafkaUtils.h"

int main() {
    drogon::app().addListener("0.0.0.0", 8080);
    
    // Test route
    drogon::app().registerHandler(
        "/",
        [](const drogon::HttpRequestPtr& req,
           std::function<void (const drogon::HttpResponsePtr &)> &&callback) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->setBody("Ride Hailing Backend is running!");
            callback(resp);
        },
        {drogon::Get}
    );

    LOG_INFO << "Starting server on 0.0.0.0:8080";
    
    // Centralized exception handling
    drogon::app().setExceptionHandler([](const std::exception &e,
                                         const drogon::HttpRequestPtr &req,
                                         std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        LOG_ERROR << "Unhandled exception: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal Server Error");
        callback(resp);
    });

    drogon::app().run();
    return 0;
}
