#include <drogon/drogon.h>
#include <postgresql/libpq-fe.h>
#include "consumers/RideEventConsumer.h"
#include <cstdlib>
#include <string>

// Helper: read env var with default
static std::string env(const char* name, const char* defaultVal) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string(defaultVal);
}

int main() {
    // Force unbuffered stdout/stderr for Docker log visibility
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // ── Logging ──────────────────────────────────────────────────────────────
    drogon::app().setLogLevel(trantor::Logger::kInfo);

    // ── Load base Drogon config (listener port etc.) ─────────────────────────
    drogon::app().loadConfigFile("config/config.json");

    // ── Database connection (env-driven, no hardcoded credentials) ───────────
    std::string pgHost     = env("PG_HOST",          "postgres");
    std::string pgUser     = env("POSTGRES_USER",    "postgres");
    std::string pgPassword = env("POSTGRES_PASSWORD","postgres");
    std::string pgDb       = env("POSTGRES_DB",      "ride_hailing");
    int         pgPort     = std::stoi(env("PG_PORT","5432"));

    // Quick libpq connection test — helps diagnose DB connectivity before Drogon starts
    LOG_INFO << "Testing libpq connection to " << pgHost << ":" << pgPort << " ...";
    std::string connStr = "host=" + pgHost + " port=" + std::to_string(pgPort)
                        + " dbname=" + pgDb + " user=" + pgUser
                        + " password=" + pgPassword + " connect_timeout=5";
    PGconn *rawConn = PQconnectdb(connStr.c_str());
    if (PQstatus(rawConn) != CONNECTION_OK) {
        LOG_ERROR << "libpq connection test FAILED: " << PQerrorMessage(rawConn);
    } else {
        LOG_INFO << "libpq connection test OK.";
    }
    PQfinish(rawConn);

    // Register Drogon DB client (connection pool)
    drogon::app().createDbClient(
        "postgresql",   // type
        pgHost,         // host
        pgPort,         // port
        pgDb,           // db name
        pgUser,         // user
        pgPassword,     // password
        5,              // connection pool size
        "",             // Unix socket path (empty = use TCP)
        "default",      // connection name
        false           // auto batch
    );

    // ── CORS ─────────────────────────────────────────────────────────────────
    // Pre-routing advice: handle OPTIONS preflight and inject CORS headers.
    drogon::app().registerPreRoutingAdvice(
        [](const drogon::HttpRequestPtr &req,
           drogon::AdviceCallback       &&stop,
           drogon::AdviceChainCallback  &&next) {
            if (req->method() == drogon::Options) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->addHeader("Access-Control-Allow-Origin",  "*");
                resp->addHeader("Access-Control-Allow-Methods", "OPTIONS, GET, POST, PUT, DELETE, PATCH");
                resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
                stop(resp);
            } else {
                next();
            }
        });

    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr &req, const drogon::HttpResponsePtr &resp) {
            resp->addHeader("Access-Control-Allow-Origin", "*");
        });

    // ── Kafka consumer ────────────────────────────────────────────────────────
    // IMPORTANT: start the consumer inside registerBeginningAdvice so it runs
    // AFTER the Drogon event loop starts and the DB connection pool is ready.
    static consumers::RideEventConsumer eventConsumer;
    drogon::app().registerBeginningAdvice([&]() {
        LOG_INFO << "Drogon is up — starting Kafka consumer...";
        eventConsumer.start();
    });

    // ── Global exception handler ──────────────────────────────────────────────
    drogon::app().setExceptionHandler(
        [](const std::exception &e,
           const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            LOG_ERROR << "Unhandled exception: " << e.what();
            Json::Value ret;
            ret["error"] = e.what();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        });

    LOG_INFO << "Starting ride-hailing-backend on 0.0.0.0:8080";
    drogon::app().run();
    return 0;
}
