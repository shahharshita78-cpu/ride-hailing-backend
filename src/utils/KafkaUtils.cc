#include "KafkaUtils.h"
#include <iostream>
#include <cstdlib>
#include <mutex>
#include <drogon/drogon.h>

namespace utils {
namespace kafka {

static std::string getBroker() {
    const char* env_broker = std::getenv("KAFKA_BROKER");
    return env_broker ? std::string(env_broker) : "kafka:9092";
}

// Thread-safe lazy producer singleton using call_once
static cppkafka::Producer& getProducer() {
    static std::once_flag initFlag;
    static std::unique_ptr<cppkafka::Producer> producer;

    std::call_once(initFlag, []() {
        cppkafka::Configuration config = {
            { "metadata.broker.list", getBroker() },
            { "message.timeout.ms",   "5000" },
            { "socket.timeout.ms",    "5000" },
            // Disable Nagle's algorithm for lower latency
            { "socket.nagle.disable", "true" }
        };
        producer = std::make_unique<cppkafka::Producer>(config);
        LOG_INFO << "Kafka producer initialized (broker: " << getBroker() << ")";
    });

    return *producer;
}

void produceEvent(const std::string& topic,
                  const std::string& key,
                  const std::string& payload) {
    try {
        auto& prod = getProducer();
        cppkafka::MessageBuilder builder(topic);
        builder.key(key).payload(payload);
        prod.produce(builder);
        // Non-blocking poll — lets rdkafka service delivery callbacks
        prod.poll(std::chrono::milliseconds(0));
    } catch (const std::exception& e) {
        // Never let Kafka errors bubble up and kill the request pipeline
        LOG_ERROR << "Kafka produceEvent error [topic=" << topic
                  << " key=" << key << "]: " << e.what();
    }
}

} // namespace kafka
} // namespace utils
