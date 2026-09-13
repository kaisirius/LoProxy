#!/bin/bash

echo "Starting backends..."
python3 -m http.server 3001 &> /dev/null &
B1=$!
python3 -m http.server 3002 &> /dev/null &
B2=$!
python3 -m http.server 3003 &> /dev/null &
B3=$!

sleep 5

echo "Starting proxy..."
./build/loproxy ./config/config.json &> /dev/null &
PROXY=$!

sleep 5

echo ""
echo "=== Benchmark: 4 threads, 100 concurrent connections, 30 seconds ==="
wrk -t4 -c100 -d30s http://localhost:8080/


kill $B1 $B2 $B3 $PROXY
wait 2>/dev/null