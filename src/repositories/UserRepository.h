#pragma once
#include <string>
#include <json/json.h>

namespace repositories {
class UserRepository {
public:
    static std::string createUser(const std::string& name, const std::string& email, const std::string& phone, const std::string& passwordHash, const std::string& role);
    static Json::Value getUserByEmail(const std::string& email);
};
}
