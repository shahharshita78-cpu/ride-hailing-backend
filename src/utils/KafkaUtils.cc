#include "KafkaUtils.h"
#include <iostream>
#include <thread>
#include <cstdlib>
#include <drogon/orm/DbClient.h>
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

void startConsumer() {
    std::thread([]() {
        try {
            cppkafka::Configuration config = {
                { "metadata.broker.list", getBroker() },
                { "group.id", "ride_events_group" },
                { "auto.offset.reset", "latest" }
            };
            cppkafka::Consumer consumer(config);
            consumer.subscribe({ "ride_events" });
            
            LOG_INFO << "Kafka consumer started on ride_events topic";
            
            while (true) {
                cppkafka::Message msg = consumer.poll();
                if (msg) {
                    if (msg.get_error()) {
                        if (!msg.is_eof()) {
                            LOG_ERROR << "Kafka consumer error: " << msg.get_error().to_string();
                        }
                    } else {
                        std::string payload(msg.get_payload());
                        size_t pos = payload.find(':');
                        if (pos != std::string::npos) {
                            std::string rideId = payload.substr(0, pos);
                            std::string status = payload.substr(pos + 1);
                            
                            auto dbClient = drogon::app().getDbClient();
                            if (dbClient) {
                                dbClient->execSqlAsync("INSERT INTO ride_events (ride_id, status) VALUES ($1, $2)",
                                    [](const drogon::orm::Result &result) {},
                                    [](const drogon::orm::DrogonDbException &e) {
                                        LOG_ERROR << "Failed to save ride event: " << e.base().what();
                                    }, rideId, status);
                                    
                                dbClient->execSqlAsync("UPDATE rides SET status = $1, updated_at = NOW() WHERE id = $2",
                                    [](const drogon::orm::Result &result) {},
                                    [](const drogon::orm::DrogonDbException &e) {
                                        LOG_ERROR << "Failed to update ride status: " << e.base().what();
                                    }, status, rideId);
                            }
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "Kafka consumer thread error: " << e.what();
        }
    }).detach();
}

}
}
