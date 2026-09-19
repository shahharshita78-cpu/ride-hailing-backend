# Uber-Style Frontend for Ride-Hailing Backend

This is a React + TypeScript frontend built with Vite and Tailwind CSS. It serves as a UI layer on top of the C++/Drogon Distributed Ride-Hailing Backend.

## Features
- **Rider View**: Request rides, view map, see live status updates, and rate drivers.
- **Driver View**: Toggle online status, receive ride requests, accept/decline, start trip, and complete trip.
- **Live Tracking**: Integrates with the backend's WebSockets to simulate driver location updates on a Leaflet map.

## Setup & Running

1. **Start the Backend**
   Ensure the C++ backend (and Postgres/Redis/Kafka) is running via `docker-compose up` in the root directory.

2. **Configure Environment**
   In this `/frontend` directory, copy `.env.example` to `.env`:
   ```bash
   cp .env.example .env
   ```

3. **Install Dependencies**
   ```bash
   npm install
   ```

4. **Run the Development Server**
   ```bash
   npm run dev
   ```

5. **Usage**
   - Open two browser tabs: `http://localhost:5173`
   - In Tab 1, Register/Login as a **PASSENGER**.
   - In Tab 2, Register/Login as a **DRIVER**.
   - As a driver, click "GO ONLINE".
   - As a passenger, enter a pickup/destination and click "Request Ride".
   - The driver will see the "New Trip" request, which they can ACCEPT.
   - The UI will sync through the ride lifecycle (MATCHED -> ONGOING -> COMPLETED).
