#include <drogon/drogon.h>
#include "utils/KafkaUtils.h"
#include "consumers/RideEventConsumer.h"
#include <cstdlib>

int main() {
    // Force unbuffered stdout/stderr
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // Enable Trace logging
    drogon::app().setLogLevel(trantor::Logger::kTrace);

    // Connect using the Drogon config file
    drogon::app().loadConfigFile("../config/config.json");

    // TEMPORARILY DISABLED: Start Kafka consumer immediately
    // static consumers::RideEventConsumer eventConsumer;
    // eventConsumer.start();

    // Add CORS support
    drogon::app().registerPreRoutingAdvice([](const drogon::HttpRequestPtr &req,
                                              drogon::FilterCallback &&defer,
                                              drogon::FilterChainCallback &&chain) {
        if (req->method() == drogon::Options) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "OPTIONS, GET, POST, PUT, DELETE, PATCH");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
            defer(resp);
        } else {
            chain();
        }
    });

    drogon::app().registerPostHandlingAdvice([](const drogon::HttpRequestPtr &req, const drogon::HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
    });

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
