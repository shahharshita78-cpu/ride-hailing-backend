# Ride Hailing Backend

## Overview
A distributed ride-hailing backend built in C++ as a portfolio project. It provides REST APIs for authentication, drivers, rides, ratings, and real-time ride updates using WebSockets.

## Features
- Passenger and driver authentication
- Ride request, acceptance, and completion flows
- Real-time driver location updates
- Ride history and active ride tracking
- Asynchronous event processing

## Architecture
- Client Application communicates with REST APIs and WebSockets.
- REST API handles HTTP requests and interfaces with PostgreSQL and Redis.
- Redis manages fast geospatial queries for driver locations.
- PostgreSQL stores persistent transactional data.
- Kafka streams ride events to consumers for asynchronous background tasks.

## Tech Stack
- C++ (Drogon Framework)
- PostgreSQL 15
- Redis 7
- Apache Kafka 3.7
- Docker / Docker Compose

## Project Structure
```
src/
├── main.cc
├── controllers/
├── filters/
├── repositories/
├── consumers/
├── utils/
└── models/
config/
└── config.json
migrations/
└── 001_init_schema.sql
docker/
└── Dockerfile
```

## Setup

```bash
git clone https://github.com/shahharshita78-cpu/ride-hailing-backend
cd ride-hailing-backend
cp .env.example .env
```

## Environment Variables
- `PG_HOST`: PostgreSQL hostname
- `PG_PORT`: PostgreSQL port
- `POSTGRES_USER`: Database user
- `POSTGRES_PASSWORD`: Database password
- `POSTGRES_DB`: Database name
- `REDIS_HOST`: Redis hostname
- `REDIS_PORT`: Redis port
- `KAFKA_BROKERS`: Kafka broker list
- `JWT_SECRET`: Secret key for JWT signing

## Running the Project
```bash
docker compose up -d --build
```

## API Endpoints

| Method | Path | Auth | Description |
|--------|------|------|-------------|
| GET | `/health` | None | Health check |
| POST | `/api/auth/register` | None | Register a new user |
| POST | `/api/auth/login` | None | Login user |
| POST | `/api/rides` | PASSENGER | Request a ride |
| GET | `/api/rides/active` | JWT | Get active ride |
| GET | `/api/rides/history` | JWT | Get ride history |
| GET | `/api/rides/{id}` | JWT | Get specific ride |
| POST | `/api/rides/{id}/accept` | DRIVER | Accept a ride |
| POST | `/api/rides/{id}/start` | DRIVER | Start a ride |
| POST | `/api/rides/{id}/complete` | DRIVER | Complete a ride |
| POST | `/api/rides/{id}/cancel` | JWT | Cancel a ride |
| POST | `/api/rides/{id}/payment` | JWT | Process payment |
| POST | `/api/rides/{id}/rating` | JWT | Submit a rating |
| GET | `/api/drivers/{id}` | None | Get driver profile |
| GET | `/api/drivers/{id}/vehicle` | None | Get driver vehicle |

## Testing
```bash
bash test_backend.sh
```

## WebSocket
Connect to `ws://localhost:8080/api/ws?token=<JWT>` for real-time ride updates and driver location broadcasting.

## Database
Uses PostgreSQL for transactional data storage, including user accounts, ride metadata, and payments.

## Redis
Used for caching and fast geospatial lookups for driver locations (using `GEOADD` and `GEORADIUS`).

## Kafka
Handles asynchronous event streaming, allowing background processing of ride events without blocking the main HTTP threads.

## Troubleshooting
If the backend does not start, ensure Docker is running and ports 8080, 5432, 6379, and 9092 are available on the host machine. Use `docker compose logs app` to inspect backend errors.
