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
    try {
        cppkafka::Configuration config = {
            { "metadata.broker.list", utils::kafka::getBroker() },
            { "group.id", "ride_event_indexer" },
            { "enable.auto.commit", true }
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

            std::string key = msg.get_key() ? msg.get_key().to_string() : "";
            std::string payload = msg.get_payload() ? msg.get_payload().to_string() : "";

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
        LOG_ERROR << "RideEventConsumer fatal error: " << e.what();
    }
}

}
