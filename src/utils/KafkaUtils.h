#pragma once
#include <string>
#include <memory>
#include <cppkafka/cppkafka.h>

namespace utils {
namespace kafka {

void produceEvent(const std::string& topic, const std::string& key, const std::string& payload);
void startConsumer();

}
}
