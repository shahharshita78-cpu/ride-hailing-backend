CREATE TABLE drivers (
    user_id UUID PRIMARY KEY REFERENCES users(id),
    vehicle_details VARCHAR(255) NOT NULL,
    is_online BOOLEAN DEFAULT false,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
