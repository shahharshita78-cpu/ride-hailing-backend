#include "RatingController.h"
#include <drogon/orm/DbClient.h>

using namespace api::rides;

void RatingController::submitRating(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("rating")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    int rating = (*jsonPtr)["rating"].asInt();
    std::string review = jsonPtr->isMember("review") ? (*jsonPtr)["review"].asString() : "";

    auto dbClient = drogon::app().getDbClient();
    try {
        dbClient->execSqlSync(
            "INSERT INTO rating (ride_id, rating, review) VALUES ($1, $2, $3)",
            id, rating, review
        );

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}
