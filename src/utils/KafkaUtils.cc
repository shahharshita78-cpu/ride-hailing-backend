#include "KafkaUtils.h"
#include <iostream>
#include <cstdlib>
#include <drogon/drogon.h>

namespace utils {
namespace kafka {

std::string getBroker() {
    const char* env_broker = std::getenv("KAFKA_BROKER");
    if (env_broker) return std::string(env_broker);
    return "localhost:9092";
}

void produceEvent(const std::string& topic, const std::string& key, const std::string& payload) {
    try {
        cppkafka::Configuration config = {
            { "metadata.broker.list", getBroker() }
        };
        cppkafka::Producer producer(config);
        
        cppkafka::MessageBuilder builder(topic);
        builder.key(key).payload(payload);
        
        producer.produce(builder);
        producer.flush();
    } catch (const std::exception& e) {
        LOG_ERROR << "Kafka producer error: " << e.what();
    }
}

}
}
