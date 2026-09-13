#!/bin/bash

echo "Building fast_backend..."
cmake --build build --target fast_backend 2>/dev/null

echo "Starting fast backends..."
./build/fast_backend 3001 &
B1=$!
./build/fast_backend 3002 &
B2=$!
./build/fast_backend 3003 &
B3=$!

sleep 5

echo "Starting proxy..."
./build/loproxy &> /tmp/proxy.log &
PROXY=$!

sleep 5

echo ""
echo "=== Benchmark: 4 threads, 50 concurrent connections, 30 seconds ==="
wrk -t4 -c50 -d30s http://localhost:8080/

echo ""
echo "Cleaning up..."
kill $B1 $B2 $B3 $PROXY 2>/dev/null
wait 2>/dev/null
echo "Done."