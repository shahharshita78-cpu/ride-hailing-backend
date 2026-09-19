#!/bin/bash
echo "=== Running Backend Diagnostic Test ==="
echo "1. Checking if containers are up..."
docker compose ps

echo -e "\n2. Capturing FULL app logs..."
docker compose logs --tail 30 app

echo -e "\n3. Capturing FULL postgres logs..."
docker compose logs --tail 30 postgres

echo -e "\n3.5 Testing network connectivity using wget (built into alpine/ubuntu?)..."
docker exec ride-hailing-backend-app-1 wget -qO- --timeout=3 http://127.0.0.1:8080/api/auth/register || echo "❌ wget failed"

echo -e "\n3.7 Testing API endpoint FROM INSIDE APP CONTAINER..."
docker exec ride-hailing-backend-app-1 curl -s -m 10 -v -X POST -H "Content-Type: application/json" -d '{"name": "test", "email": "test_api_check@gmail.com", "phone": "1234569999", "password": "abc", "role": "PASSENGER"}' http://127.0.0.1:8080/api/auth/register

echo -e "\n3. Testing API endpoint (with 10-second timeout)..."
curl -4 -s -m 10 -v -X POST -H "Content-Type: application/json" -d '{"name": "test", "email": "test_api_check@gmail.com", "phone": "1234569999", "password": "abc", "role": "PASSENGER"}' http://127.0.0.1:8080/api/auth/register > response.txt
CURL_STATUS=$?

echo -e "\n\n4. Results:"
if [ $CURL_STATUS -eq 28 ]; then
    echo "❌ ERROR: Request HUNG and TIMED OUT after 10 seconds!"
    echo "The backend is running but stuck waiting for a database connection (execSqlSync blocked)."
elif [ $CURL_STATUS -eq 7 ]; then
    echo "❌ ERROR: Connection Refused!"
    echo "The backend server is completely dead or still compiling."
elif [ $CURL_STATUS -eq 0 ]; then
    echo "✅ Request completed!"
    echo "Response from server:"
    cat response.txt
else
    echo "❌ Request failed with curl exit code $CURL_STATUS"
fi
