#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
namespace rides {

class RatingController : public drogon::HttpController<RatingController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RatingController::submitRating, "/api/rides/{id}/rating", drogon::Post, "JwtFilter");
    METHOD_LIST_END

    void submitRating(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
};

}
}
