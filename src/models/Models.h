#pragma once

#include <string>
#include <optional>
#include <vector>
#include <json/json.h>

namespace models {

struct User {
    std::string user_id;
    std::string name;
    std::string email;
    std::string phone;
    std::string password_hash;
    std::string role;
    std::string created_at;
    std::string updated_at;
};

struct Passenger {
    std::string passenger_id;
    std::string user_id;
    int total_rides;
    std::string created_at;
    std::string updated_at;
};

struct Driver {
    std::string driver_id;
    std::string user_id;
    std::string current_status;
    double avg_rating;
    std::string created_at;
    std::string updated_at;
};

struct Ride {
    std::string ride_id;
    std::string passenger_id;
    std::optional<std::string> driver_id;
    std::string pickup;
    std::string destination;
    std::optional<double> distance_km;
    std::optional<int> estimated_time;
    std::optional<double> estimated_fare;
    std::optional<double> final_fare;
    std::string ride_status;
    std::string requested_at;
    std::optional<std::string> accepted_at;
    std::optional<std::string> started_at;
    std::optional<std::string> completed_at;
    std::string created_at;
    std::string updated_at;
};

struct Payment {
    std::string payment_id;
    std::string ride_id;
    double amount;
    std::string payment_method;
    std::string payment_status;
    std::optional<std::string> transaction_id;
    std::optional<std::string> paid_at;
    std::string created_at;
    std::string updated_at;
};

struct Rating {
    std::string rating_id;
    std::string ride_id;
    std::string passenger_id;
    std::string driver_id;
    int rating;
    std::optional<std::string> review;
    std::string created_at;
};

struct DriverLocation {
    std::string location_id;
    std::string driver_id;
    double latitude;
    double longitude;
    std::string recorded_at;
};

struct Vehicle {
    std::string vehicle_id;
    std::string driver_id;
    std::string vehicle_number;
    std::string vehicle_type;
    std::string vehicle_model;
    std::string vehicle_color;
    std::string created_at;
    std::string updated_at;
};

} // namespace models
