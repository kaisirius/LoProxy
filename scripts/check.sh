#!/usr/bin/env bash

set -e

BUILD_DIR="build"
SERVER_BIN="./build/loproxy"
PORT=8080
TEST_MSG="hello"

echo "[INFO] Creating build directory..."
mkdir -p "$BUILD_DIR"

echo "[INFO] Running CMake..."
cmake -S . -B "$BUILD_DIR"

echo "[INFO] Building project..."
cmake --build "$BUILD_DIR"

if [ $? -ne 0 ]; then
    echo "[FAIL] Build failed"
    exit 1
fi

echo "[INFO] Starting server..."
"$SERVER_BIN" &
SERVER_PID=$!


sleep 1

echo "[INFO] Sending test message..."

RESPONSE=$(echo "$TEST_MSG" | nc -N 127.0.0.1 "$PORT")

echo "[INFO] Stopping server..."
kill "$SERVER_PID" 2>/dev/null
wait "$SERVER_PID" 2>/dev/null || true

if [ "$RESPONSE" = "$TEST_MSG" ]; then
    echo "PASS"
    exit 0
else
    echo "FAIL"
    echo "Expected: $TEST_MSG"
    echo "Received: $RESPONSE"
    exit 1
fi