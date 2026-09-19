#include "JwtUtils.h"
#include <jwt-cpp/jwt.h>
#include <cstdlib>
#include <iostream>
#include <chrono>

namespace utils {
namespace jwt_utils {

std::string getSecret() {
    const char* env_secret = std::getenv("JWT_SECRET");
    if (env_secret) return std::string(env_secret);
    return "super_secret_key_change_me_in_prod";
}

std::string generateToken(const std::string& userId, const std::string& role) {
    auto token = jwt::create()
        .set_issuer("ride_hailing_backend")
        .set_type("JWS")
        .set_payload_claim("user_id", jwt::claim(std::string(userId)))
        .set_payload_claim("role", jwt::claim(std::string(role)))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
        .sign(jwt::algorithm::hs256{getSecret()});
        
    return token;
}

bool verifyToken(const std::string& token, std::string& outUserId, std::string& outRole) {
    try {
        auto decoded = jwt::decode(token);
        
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{getSecret()})
            .with_issuer("ride_hailing_backend");
            
        verifier.verify(decoded);
        
        outUserId = decoded.get_payload_claim("user_id").as_string();
        outRole = decoded.get_payload_claim("role").as_string();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "JWT Verification failed: " << e.what() << std::endl;
        return false;
    }
}

}
}
