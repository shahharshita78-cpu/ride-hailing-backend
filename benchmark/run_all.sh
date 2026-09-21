#!/bin/bash
echo "========================================================="
echo "   RIDE-HAILING BACKEND PERFORMANCE VALIDATION SUITE     "
echo "========================================================="

echo -e "\nRunning Phase 1: API Latency Test..."
node latency_test.js

echo -e "\nRunning Phase 2: Database Stress Test..."
node db_stress.js

echo -e "\nRunning Phase 3: Kafka & WebSockets Test..."
node ws_kafka_test.js

echo -e "\n========================================================="
echo "   BENCHMARK COMPLETE. CAPTURE THIS OUTPUT FOR PROOF!    "
echo "========================================================="
