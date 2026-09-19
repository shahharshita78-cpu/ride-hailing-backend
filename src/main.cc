#include <drogon/drogon.h>
#include "utils/KafkaUtils.h"
#include "consumers/RideEventConsumer.h"
#include <cstdlib>

int main() {
    // Connect to PostgreSQL using environment variables
    const char* pg_user   = std::getenv("POSTGRES_USER");
    const char* pg_pass   = std::getenv("POSTGRES_PASSWORD");
    const char* pg_db     = std::getenv("POSTGRES_DB");

    const char* pg_host = "postgres";
    int pg_port = 5432;
    std::string final_db = pg_db ? pg_db : "ride_hailing";
    std::string final_user = pg_user ? pg_user : "postgres";

    LOG_INFO << "Configuring DB client: host=" << pg_host << " port=" << pg_port 
             << " dbname=" << final_db << " user=" << final_user;

    // Initialize DB client
    drogon::app().createDbClient(
        "postgresql",
        pg_host,
        pg_port,
        final_db,
        final_user,
        pg_pass ? pg_pass : "postgres",
        5,                    // connection pool size
        "",                   // unix socket (empty = TCP)
        "default"             // client name
    );

    drogon::app().addListener("0.0.0.0", 8080);

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
