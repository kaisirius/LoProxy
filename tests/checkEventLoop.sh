\#!/usr/bin/env bash

set -e

BUILD_DIR="build"
SERVER_BIN="./build/loproxy"
PORT=8080
NUM_CLIENTS=50

TMP_DIR=$(mktemp -d)

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
    echo
    echo "[INFO] Cleaning up..."

    # Kill any remaining clients
    if [ ${#CLIENT_PIDS[@]} -gt 0 ]; then
        kill "${CLIENT_PIDS[@]}" 2>/dev/null || true

        for pid in "${CLIENT_PIDS[@]}"; do
            wait "$pid" 2>/dev/null || true
        done
    fi

    echo "[INFO] Stopping server..."

    kill "$SERVER_PID" 2>/dev/null || true
    sleep 1

    if kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "[WARN] Force killing server..."
        kill -9 "$SERVER_PID" 2>/dev/null || true
    fi

    wait "$SERVER_PID" 2>/dev/null || true

    rm -rf "$TMP_DIR"
}

trap cleanup EXIT INT TERM

sleep 1

echo "[INFO] Launching $NUM_CLIENTS concurrent clients..."

for ((i=1; i<=NUM_CLIENTS; i++)); do
(
    MSG="hello-world-$i"

    {
        printf "%s" "$MSG"
        sleep 5
    } | nc 127.0.0.1 "$PORT" > "$TMP_DIR/client_$i.out"
) &
    CLIENT_PIDS+=($!)
done

echo "[INFO] Waiting for responses..."
sleep 6

echo "[INFO] Stopping client processes..."

kill "${CLIENT_PIDS[@]}" 2>/dev/null || true

for pid in "${CLIENT_PIDS[@]}"; do
    wait "$pid" 2>/dev/null || true
done

echo "[INFO] Verifying responses..."

FAIL=0

for ((i=1; i<=NUM_CLIENTS; i++)); do
    EXPECTED="hello-world-$i"

    if [ -f "$TMP_DIR/client_$i.out" ]; then
        RESPONSE=$(tr -d '\r\n' < "$TMP_DIR/client_$i.out")
    else
        RESPONSE=""
    fi

    if [ "$RESPONSE" = "$EXPECTED" ]; then
        echo "[PASS] Client $i"
    else
        echo "[FAIL] Client $i"
        echo "       Expected: $EXPECTED"
        echo "       Received: $RESPONSE"
        FAIL=1
    fi
done

echo

if [ "$FAIL" -eq 0 ]; then
    echo "[PASS] All $NUM_CLIENTS clients received correct responses."
else
    echo "[FAIL] Some clients did not receive the expected response."
    exit 1
fi