#include "PaymentController.h"
#include "../utils/KafkaUtils.h"
#include <drogon/orm/DbClient.h>

using namespace api::rides;

void PaymentController::processPayment(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("amount") || !jsonPtr->isMember("payment_method")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    double amount = (*jsonPtr)["amount"].asDouble();
    std::string method = (*jsonPtr)["payment_method"].asString();

    auto dbClient = drogon::app().getDbClient();
    try {
        dbClient->execSqlSync(
            "INSERT INTO payment (ride_id, amount, payment_method, payment_status, paid_at) VALUES ($1, $2, CAST($3 AS payment_method_enum), 'SUCCESS', CURRENT_TIMESTAMP)",
            id, amount, method
        );

        ::utils::kafka::produceEvent("ride_events", id, "PaymentCompleted");

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setBody("Payment processed");
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}
