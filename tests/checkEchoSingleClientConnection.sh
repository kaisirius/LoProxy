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

echo "[INFO] Starting server..."
"$SERVER_BIN" &
SERVER_PID=$!

cleanup() {
    echo "[INFO] Stopping server..."

    kill "$SERVER_PID" 2>/dev/null || true

    # Give the server a chance to exit gracefully.
    sleep 1

    # If it's still running, force kill it.
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "[WARN] Server did not exit gracefully. Force killing..."
        kill -9 "$SERVER_PID" 2>/dev/null || true
    fi

    wait "$SERVER_PID" 2>/dev/null || true
}

trap cleanup EXIT INT TERM

sleep 1

echo "[INFO] Sending test message..."

RESPONSE=$(echo "$TEST_MSG" | nc -N 127.0.0.1 "$PORT")

if [ "$RESPONSE" = "$TEST_MSG" ]; then
    echo "PASS"
    exit 0
else
    echo "FAIL"
    echo "Expected: $TEST_MSG"
    echo "Received: $RESPONSE"
    exit 1
fi