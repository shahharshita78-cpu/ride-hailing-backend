#include "HealthController.h"
#include <drogon/drogon.h>
#include <json/json.h>

using namespace api::health;

void HealthController::check(const drogon::HttpRequestPtr &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    Json::Value ret;
    ret["status"]  = "ok";
    ret["service"] = "ride-hailing-backend";

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k200OK);
    callback(resp);
}
