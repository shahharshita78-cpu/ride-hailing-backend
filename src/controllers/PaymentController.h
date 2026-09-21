#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
namespace rides {

class PaymentController : public drogon::HttpController<PaymentController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PaymentController::processPayment, "/api/rides/{id}/payment", drogon::Post, "JwtFilter");
    METHOD_LIST_END

    void processPayment(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string id);
};

}
}
