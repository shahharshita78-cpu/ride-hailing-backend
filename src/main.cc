#include <drogon/drogon.h>
#include "utils/KafkaUtils.h"
#include "consumers/RideEventConsumer.h"
#include <cstdlib>

int main() {
    // Connect using the Drogon config file
    drogon::app().loadConfigFile("../config/config.json");

    // Start Kafka consumer only after Drogon has connected to the DB
    static consumers::RideEventConsumer eventConsumer;
    drogon::app().registerBeginningAdvice([]() {
        auto checkDb = std::make_shared<std::function<void(int)>>();
        *checkDb = [checkDb](int attempt) {
            auto dbClient = drogon::app().getDbClient();
            dbClient->execSqlAsync(
                "SELECT 1",
                [checkDb](const drogon::orm::Result& result) {
                    LOG_INFO << "DB connectivity OK";
                    eventConsumer.start();
                },
                [checkDb, attempt](const drogon::orm::DrogonDbException& e) {
                    LOG_ERROR << "DB connectivity FAILED: " << e.base().what();
                    if (attempt < 10) {
                        LOG_INFO << "Retrying DB connectivity check in 3 seconds (attempt " << attempt + 1 << " of 10)...";
                        drogon::app().getLoop()->runAfter(3.0, [checkDb, attempt]() {
                            (*checkDb)(attempt + 1);
                        });
                    } else {
                        LOG_ERROR << "Failed to connect to DB after 10 attempts";
                    }
                }
            );
        };
        (*checkDb)(1);
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
