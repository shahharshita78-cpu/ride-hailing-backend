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
        static cppkafka::Producer* producer = nullptr;
        if (!producer) {
            try {
                cppkafka::Configuration config = {
                    { "metadata.broker.list", getBroker() },
                    { "message.timeout.ms", 3000 },
                    { "socket.timeout.ms", 3000 }
                };
                producer = new cppkafka::Producer(config);
            } catch (const std::exception& e) {
                LOG_ERROR << "Failed to create Kafka producer: " << e.what();
                return;
            }
        }
        
        cppkafka::MessageBuilder builder(topic);
        builder.key(key).payload(payload);
        
        producer->produce(builder);
        producer->poll(std::chrono::milliseconds(0));
        producer->flush(std::chrono::milliseconds(100));
    } catch (const std::exception& e) {
        LOG_ERROR << "Kafka producer error: " << e.what();
    }
}

}
}
