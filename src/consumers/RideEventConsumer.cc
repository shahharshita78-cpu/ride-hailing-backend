#include "RideEventConsumer.h"
#include <drogon/drogon.h>
#include "../utils/KafkaUtils.h"

namespace consumers {

RideEventConsumer::RideEventConsumer() {}

RideEventConsumer::~RideEventConsumer() {
    stop();
}

void RideEventConsumer::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&RideEventConsumer::run, this);
}

void RideEventConsumer::stop() {
    if (!running_) return;
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void RideEventConsumer::run() {
    while (running_) {
        try {
            const char* envBroker = std::getenv("KAFKA_BROKER");
            std::string broker = envBroker ? envBroker : "kafka:9092";

            cppkafka::Configuration config = {
                { "metadata.broker.list", broker },
                { "group.id", "ride_event_indexer" },
                { "enable.auto.commit", true },
                { "auto.offset.reset", "earliest" }
            };

            cppkafka::Consumer consumer(config);
            consumer.subscribe({ "ride_events" });
            
            LOG_INFO << "RideEventConsumer started consuming from ride_events";

            while (running_) {
                cppkafka::Message msg = consumer.poll(std::chrono::milliseconds(500));
                if (!msg) {
                    continue;
                }

                if (msg.get_error()) {
                    if (!msg.is_eof()) {
                        LOG_ERROR << "Kafka consume error: " << msg.get_error().to_string();
                    }
                    continue;
                }

                std::string key = msg.get_key() ? std::string((const char*)msg.get_key().get_data(), msg.get_key().get_size()) : "";
                std::string payload = msg.get_payload() ? std::string((const char*)msg.get_payload().get_data(), msg.get_payload().get_size()) : "";

                if (!key.empty() && !payload.empty()) {
                    try {
                        auto dbClient = drogon::app().getDbClient();
                        if (!dbClient) {
                            LOG_ERROR << "RideEventConsumer: DB client not available, skipping event: " << key;
                            continue;
                        }
                        dbClient->execSqlSync(
                            "INSERT INTO ride_event (ride_id, event_type) VALUES ($1, $2)",
                            key, payload
                        );
                        LOG_DEBUG << "Indexed ride event: " << key << " -> " << payload;
                    } catch (const std::exception& dbEx) {
                        LOG_ERROR << "Failed to insert ride_event to DB: " << dbEx.what();
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "RideEventConsumer error (will retry in 5s): " << e.what();
            if (running_) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
}

}
