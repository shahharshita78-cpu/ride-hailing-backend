#pragma once
#include <string>

namespace utils {
namespace crypto {

std::string hashPassword(const std::string& password, const std::string& salt = "");
bool verifyPassword(const std::string& password, const std::string& hash);
std::string generateSalt(size_t length = 16);

}
}
