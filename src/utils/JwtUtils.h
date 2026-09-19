#pragma once
#include <string>

namespace utils {
namespace jwt_utils {

std::string generateToken(const std::string& userId, const std::string& role);
bool verifyToken(const std::string& token, std::string& outUserId, std::string& outRole);

}
}
