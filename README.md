# Distributed Ride-Hailing Backend

A distributed ride-hailing backend built in C++ as a portfolio project. It emphasizes clean architecture and handles core flows such as authentication, driver management, ride lifecycle state machine, and real-time location updates.

## Tech Stack
C++, Drogon, PostgreSQL, Redis, Apache Kafka, WebSockets, Docker, JWT, REST APIs

## Architecture Overview
```mermaid
graph TD
    Client[Client App] --> API[Drogon REST APIs]
    Client <--> WS[Drogon WebSockets]
    API --> DB[(PostgreSQL)]
    API --> Cache[(Redis)]
    API --> Kafka[Kafka Topic]
    Kafka --> Consumer[Kafka Consumer]
    Consumer --> DB
    WS --> Cache
```

## Setup Instructions

1. **Clone the repository**
2. **Copy `.env.example` to `.env`** and update variables if necessary.
3. **Run using Docker Compose:**
   ```bash
   docker-compose up --build
   ```

## Current Status / Roadmap
- [x] Phase 1: Project Scaffolding
- [ ] Phase 2: Auth Service
- [ ] Phase 3: Driver Management
- [ ] Phase 4: Ride Lifecycle & Real-time Updates
- [ ] Phase 5: Final Polish

## API Documentation
(To be updated as endpoints are implemented)
