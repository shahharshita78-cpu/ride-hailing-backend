#include <drogon/drogon.h>
#include "utils/KafkaUtils.h"
#include "consumers/RideEventConsumer.h"
#include <cstdlib>

int main() {
    // Load config file — this creates the DB client from config/config.json
    // which reads host "postgres" and credentials from environment variables
    // via docker-compose: POSTGRES_USER / POSTGRES_PASSWORD / POSTGRES_DB
    // Override the static config values with env vars if present
    const char* pg_user   = std::getenv("POSTGRES_USER");
    const char* pg_pass   = std::getenv("POSTGRES_PASSWORD");
    const char* pg_db     = std::getenv("POSTGRES_DB");

    // createDbClient programmatically so env vars take precedence over config.json
    drogon::app().createDbClient(
        "postgresql",
        "postgres",           // host — matches the docker-compose service name
        5432,
        pg_db   ? pg_db   : "ride_hailing",
        pg_user ? pg_user : "postgres",
        pg_pass ? pg_pass : "postgres",
        5,                    // connection pool size
        "",                   // unix socket (empty = TCP)
        "default"             // client name
    );

    drogon::app().addListener("0.0.0.0", 8080);

    // Start Kafka consumer only after Drogon has connected to the DB
    // registerBeginningAdvice is called once, right after app().run() initialises
    // all plugins and DB connections, before serving any requests.
    static consumers::RideEventConsumer eventConsumer;
    drogon::app().registerBeginningAdvice([&]() {
        eventConsumer.start();
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
