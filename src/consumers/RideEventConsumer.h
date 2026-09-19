#pragma once
#include <cppkafka/cppkafka.h>
#include <atomic>
#include <thread>
#include <string>

namespace consumers {

class RideEventConsumer {
public:
    RideEventConsumer();
    ~RideEventConsumer();

    void start();
    void stop();

private:
    void run();

    std::atomic<bool> running_{false};
    std::thread thread_;
};

}
