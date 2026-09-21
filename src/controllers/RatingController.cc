#include "RatingController.h"
#include <drogon/orm/DbClient.h>

using namespace api::rides;

void RatingController::submitRating(const HttpRequestPtr &req,
                                    std::function<void(const HttpResponsePtr &)> &&callback,
                                    std::string id) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("rating")) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Missing 'rating' field");
        callback(resp);
        return;
    }

    int rating = (*jsonPtr)["rating"].asInt();
    if (rating < 1 || rating > 5) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Rating must be between 1 and 5");
        callback(resp);
        return;
    }
    std::string review = jsonPtr->isMember("review") ? (*jsonPtr)["review"].asString() : "";

    auto dbClient = drogon::app().getDbClient();
    try {
        // Look up the passenger_id and driver_id for this ride —
        // required by the schema's FK constraints on the rating table.
        auto rideRes = dbClient->execSqlSync(
            "SELECT passenger_id, driver_id FROM ride WHERE ride_id = $1", id);
        if (rideRes.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k404NotFound);
            resp->setBody("Ride not found");
            callback(resp);
            return;
        }
        std::string passengerId = rideRes[0]["passenger_id"].as<std::string>();
        std::string driverId    = rideRes[0]["driver_id"].isNull()
                                  ? "" : rideRes[0]["driver_id"].as<std::string>();

        if (driverId.empty()) {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k409Conflict);
            resp->setBody("Cannot rate a ride with no driver assigned");
            callback(resp);
            return;
        }

        dbClient->execSqlSync(
            "INSERT INTO rating (ride_id, passenger_id, driver_id, rating, review) "
            "VALUES ($1, $2, $3, $4, $5)",
            id, passengerId, driverId, rating, review
        );

        Json::Value ret;
        ret["message"] = "Rating submitted";
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        resp->setStatusCode(k200OK);
        callback(resp);
    } catch (const std::exception &e) {
        LOG_ERROR << "submitRating error: " << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}
