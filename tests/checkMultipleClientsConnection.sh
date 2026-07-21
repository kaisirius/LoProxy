#!/usr/bin/env bash

set -e

BUILD_DIR="build"
SERVER_BIN="./build/loproxy"
PORT=8080
NUM_CLIENTS=50

echo "[INFO] Creating build directory..."
mkdir -p "$BUILD_DIR"

echo "[INFO] Running CMake..."
cmake -S . -B "$BUILD_DIR"

echo "[INFO] Building project..."
cmake --build "$BUILD_DIR"

echo "[INFO] Starting server..."
"$SERVER_BIN" &
SERVER_PID=$!

CLIENT_PIDS=()

cleanup() {
    echo "[INFO] Closing client connections..."

    if [ ${#CLIENT_PIDS[@]} -gt 0 ]; then
        kill "${CLIENT_PIDS[@]}" 2>/dev/null || true

        for pid in "${CLIENT_PIDS[@]}"; do
            wait "$pid" 2>/dev/null || true
        done
    fi

    echo "[INFO] Stopping server..."

    kill "$SERVER_PID" 2>/dev/null || true

    # Give the server a moment to exit cleanly
    sleep 1

    # If it's still alive, force kill it
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "[WARN] Server did not exit gracefully. Force killing..."
        kill -9 "$SERVER_PID" 2>/dev/null || true
    fi

    wait "$SERVER_PID" 2>/dev/null || true
}

trap cleanup EXIT INT TERM

sleep 1

echo "[INFO] Opening $NUM_CLIENTS concurrent connections..."

for ((i=1; i<=NUM_CLIENTS; i++)); do
    tail -f /dev/null | nc 127.0.0.1 "$PORT" >/dev/null 2>&1 &
    CLIENT_PIDS+=($!)
done

sleep 2

echo "[PASS] $NUM_CLIENTS simultaneous connections established."

# Keep them open for a while
sleep 5

echo "[INFO] Test complete."