#include "JwtFilter.h"
#include "../utils/JwtUtils.h"

void JwtFilter::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb) {
    std::string authHeader = req->getHeader("Authorization");
    
    if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Missing or invalid Authorization header");
        fcb(resp);
        return;
    }
    
    std::string token = authHeader.substr(7);
    std::string userId, role;
    
    if (utils::jwt_utils::verifyToken(token, userId, role)) {
        req->getAttributes()->insert("user_id", userId);
        req->getAttributes()->insert("role", role);
        fccb();
    } else {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        fcb(resp);
    }
}
