CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- 1. USER
CREATE TYPE user_role AS ENUM ('PASSENGER', 'DRIVER');

CREATE TABLE users (
    user_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    phone VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role user_role NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 2. PASSENGER
CREATE TABLE passenger (
    passenger_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID UNIQUE REFERENCES users(user_id) ON DELETE CASCADE,
    total_rides INTEGER DEFAULT 0,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 3. DRIVER
CREATE TYPE driver_status AS ENUM ('OFFLINE', 'AVAILABLE', 'BUSY');

CREATE TABLE driver (
    driver_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID UNIQUE REFERENCES users(user_id) ON DELETE CASCADE,
    current_status driver_status DEFAULT 'OFFLINE',
    avg_rating DOUBLE PRECISION DEFAULT 0.0,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 4. RIDE
CREATE TYPE ride_state AS ENUM (
    'REQUESTED',
    'MATCHED',
    'DRIVER_ARRIVING',
    'ONGOING',
    'COMPLETED',
    'CANCELLED'
);

CREATE TABLE ride (
    ride_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    passenger_id UUID REFERENCES passenger(passenger_id),
    driver_id UUID REFERENCES driver(driver_id), -- NULL initially
    pickup VARCHAR(255) NOT NULL, -- Simplified as string for this demo, usually PostGIS GEOMETRY
    destination VARCHAR(255) NOT NULL,
    distance_km DOUBLE PRECISION,
    estimated_time INTEGER, -- seconds
    estimated_fare DOUBLE PRECISION,
    final_fare DOUBLE PRECISION,
    ride_status ride_state DEFAULT 'REQUESTED',
    requested_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    accepted_at TIMESTAMP WITH TIME ZONE,
    started_at TIMESTAMP WITH TIME ZONE,
    completed_at TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 5. PAYMENT
CREATE TYPE payment_method_enum AS ENUM ('UPI', 'CARD', 'CASH', 'WALLET');
CREATE TYPE payment_status_enum AS ENUM ('PENDING', 'SUCCESS', 'FAILED', 'REFUNDED');

CREATE TABLE payment (
    payment_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    ride_id UUID UNIQUE REFERENCES ride(ride_id),
    amount DOUBLE PRECISION NOT NULL,
    payment_method payment_method_enum NOT NULL,
    payment_status payment_status_enum DEFAULT 'PENDING',
    transaction_id VARCHAR(255) UNIQUE,
    paid_at TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 6. RATING
CREATE TABLE rating (
    rating_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    ride_id UUID UNIQUE REFERENCES ride(ride_id),
    passenger_id UUID REFERENCES passenger(passenger_id),
    driver_id UUID REFERENCES driver(driver_id),
    rating INTEGER CHECK (rating >= 1 AND rating <= 5),
    review TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 7. DRIVER_LOCATION
CREATE TABLE driver_location (
    location_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    driver_id UUID REFERENCES driver(driver_id),
    latitude DOUBLE PRECISION NOT NULL,
    longitude DOUBLE PRECISION NOT NULL,
    recorded_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 8. VEHICLE
CREATE TABLE vehicle (
    vehicle_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    driver_id UUID UNIQUE REFERENCES driver(driver_id),
    vehicle_number VARCHAR(50) UNIQUE NOT NULL,
    vehicle_type VARCHAR(50) NOT NULL,
    vehicle_model VARCHAR(100) NOT NULL,
    vehicle_color VARCHAR(50) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
