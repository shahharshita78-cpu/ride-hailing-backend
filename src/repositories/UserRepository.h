#pragma once
#include <string>
#include <json/json.h>

namespace repositories {
class UserRepository {
public:
    static void createUser(const std::string& name, const std::string& email, const std::string& phone, const std::string& passwordHash, const std::string& role,
                           std::function<void(const std::string&)> onSuccess, std::function<void(const std::exception&)> onError);
    static void getUserByEmail(const std::string& email, 
                               std::function<void(const Json::Value&)> onSuccess, std::function<void(const std::exception&)> onError);
};
}
