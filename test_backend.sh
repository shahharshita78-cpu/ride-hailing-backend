#!/bin/bash
# ─────────────────────────────────────────────────────────────────────────────
# test_backend.sh  —  End-to-end smoke test for ride-hailing-backend
# Usage: bash test_backend.sh
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

BASE_URL="http://127.0.0.1:8080"
PASS=0
FAIL=0

color_ok()   { echo -e "\033[0;32m $*\033[0m"; }
color_err()  { echo -e "\033[0;31m $*\033[0m"; }
color_info() { echo -e "\033[0;34mℹ  $*\033[0m"; }

assert_status() {
    local label="$1" expected="$2" actual="$3"
    if [ "$actual" = "$expected" ]; then
        color_ok  "$label (HTTP $actual)"
        PASS=$((PASS + 1))
    else
        color_err "$label — expected HTTP $expected, got $actual"
        FAIL=$((FAIL + 1))
    fi
}

# ── 1. Health check ───────────────────────────────────────────────────────────
color_info "1. Checking /health ..."
STATUS=$(curl -s -o /dev/null -w "%{http_code}" -m 5 "$BASE_URL/health")
assert_status "/health" 200 "$STATUS"

BODY=$(curl -s -m 5 "$BASE_URL/health")
if echo "$BODY" | grep -q '"ok"'; then
    color_ok  "/health body contains status=ok"
    PASS=$((PASS + 1))
else
    color_err "/health body unexpected: $BODY"
    FAIL=$((FAIL + 1))
fi

# ── 2. Register a passenger ───────────────────────────────────────────────────
color_info "2. Registering a passenger ..."
TS=$(date +%s)
REG_EMAIL="passenger_${TS}@test.com"
REG_RESP=$(curl -s -m 10 -X POST "$BASE_URL/api/auth/register" \
    -H "Content-Type: application/json" \
    -d "{\"name\":\"Test Passenger\",\"email\":\"$REG_EMAIL\",\"phone\":\"9${TS:0:9}\",\"password\":\"secret123\",\"role\":\"PASSENGER\"}")
REG_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -m 10 -X POST "$BASE_URL/api/auth/register" \
    -H "Content-Type: application/json" \
    -d "{\"name\":\"Dup Passenger\",\"email\":\"dup_${TS}@test.com\",\"phone\":\"8${TS:0:9}\",\"password\":\"secret123\",\"role\":\"PASSENGER\"}" 2>/dev/null || echo "000")
PASS_TOKEN=$(echo "$REG_RESP" | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
if [ -n "$PASS_TOKEN" ]; then
    color_ok  "Passenger registered, token received"
    PASS=$((PASS + 1))
else
    color_err "Passenger registration failed: $REG_RESP"
    FAIL=$((FAIL + 1))
fi

# ── 3. Register a driver ──────────────────────────────────────────────────────
color_info "3. Registering a driver ..."
DRV_EMAIL="driver_${TS}@test.com"
DRV_RESP=$(curl -s -m 10 -X POST "$BASE_URL/api/auth/register" \
    -H "Content-Type: application/json" \
    -d "{\"name\":\"Test Driver\",\"email\":\"$DRV_EMAIL\",\"phone\":\"7${TS:0:9}\",\"password\":\"secret123\",\"role\":\"DRIVER\"}")
DRV_TOKEN=$(echo "$DRV_RESP" | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
if [ -n "$DRV_TOKEN" ]; then
    color_ok  "Driver registered, token received"
    PASS=$((PASS + 1))
else
    color_err "Driver registration failed: $DRV_RESP"
    FAIL=$((FAIL + 1))
fi

# ── 4. Login ──────────────────────────────────────────────────────────────────
color_info "4. Logging in as passenger ..."
LOGIN_RESP=$(curl -s -m 10 -X POST "$BASE_URL/api/auth/login" \
    -H "Content-Type: application/json" \
    -d "{\"email\":\"$REG_EMAIL\",\"password\":\"secret123\"}")
LOGIN_TOKEN=$(echo "$LOGIN_RESP" | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
if [ -n "$LOGIN_TOKEN" ]; then
    color_ok  "Login successful"
    PASS=$((PASS + 1))
    PASS_TOKEN="$LOGIN_TOKEN"  # use fresh token
else
    color_err "Login failed: $LOGIN_RESP"
    FAIL=$((FAIL + 1))
fi

# ── 5. Request a ride ─────────────────────────────────────────────────────────
color_info "5. Requesting a ride ..."
RIDE_RESP=$(curl -s -m 10 -X POST "$BASE_URL/api/rides" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer $PASS_TOKEN" \
    -d '{"pickup":"Airport","destination":"Downtown","distance_km":12.5,"estimated_fare":250.0}')
RIDE_ID=$(echo "$RIDE_RESP" | grep -o '"ride_id":"[^"]*"' | cut -d'"' -f4)
if [ -n "$RIDE_ID" ]; then
    color_ok  "Ride requested: $RIDE_ID"
    PASS=$((PASS + 1))
else
    color_err "Ride request failed: $RIDE_RESP"
    FAIL=$((FAIL + 1))
fi

# ── 6. Get ride details ───────────────────────────────────────────────────────
if [ -n "$RIDE_ID" ]; then
    color_info "6. Getting ride details ..."
    RIDE_DETAIL=$(curl -s -m 10 "$BASE_URL/api/rides/$RIDE_ID" \
        -H "Authorization: Bearer $PASS_TOKEN")
    if echo "$RIDE_DETAIL" | grep -q '"REQUESTED"'; then
        color_ok  "GET /api/rides/:id works, status=REQUESTED"
        PASS=$((PASS + 1))
    else
        color_err "GET /api/rides/:id unexpected: $RIDE_DETAIL"
        FAIL=$((FAIL + 1))
    fi
fi

# ── 7. Accept ride as driver ──────────────────────────────────────────────────
if [ -n "$RIDE_ID" ] && [ -n "$DRV_TOKEN" ]; then
    color_info "7. Driver accepting ride ..."
    ACCEPT_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -m 10 \
        -X POST "$BASE_URL/api/rides/$RIDE_ID/accept" \
        -H "Authorization: Bearer $DRV_TOKEN")
    assert_status "POST /api/rides/:id/accept" 200 "$ACCEPT_STATUS"
fi

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "══════════════════════════════════════════"
echo "  Results: ${PASS} passed  |  ${FAIL} failed"
echo "══════════════════════════════════════════"
if [ "$FAIL" -eq 0 ]; then
    color_ok  "ALL TESTS PASSED"
    exit 0
else
    color_err "$FAIL TEST(S) FAILED"
    exit 1
fi
