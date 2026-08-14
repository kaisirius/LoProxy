#!/usr/bin/env bash

set -e

BUILD_DIR="build"
SERVER_BIN="./build/loproxy"
PORT=8080
NUM_CLIENTS=50
TIMEOUT=10

TMP_DIR=$(mktemp -d)

CLIENT_PIDS=()
SERVER_PID=""

cleanup() {
    echo
    echo "[INFO] Cleaning up..."

    # Kill any clients that are still running
    if [ ${#CLIENT_PIDS[@]} -gt 0 ]; then
        kill "${CLIENT_PIDS[@]}" 2>/dev/null || true

        for pid in "${CLIENT_PIDS[@]}"; do
            wait "$pid" 2>/dev/null || true
        done
    fi

    # Stop server
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "[INFO] Stopping server..."
        kill "$SERVER_PID" 2>/dev/null || true

        sleep 1

        # Force kill if still alive
        if kill -0 "$SERVER_PID" 2>/dev/null; then
            echo "[WARN] Force killing server..."
            kill -9 "$SERVER_PID" 2>/dev/null || true
        fi

        wait "$SERVER_PID" 2>/dev/null || true
    fi

    rm -rf "$TMP_DIR"
}

trap cleanup EXIT INT TERM

echo "[INFO] Creating build directory..."
mkdir -p "$BUILD_DIR"

echo "[INFO] Running CMake..."
cmake -S . -B "$BUILD_DIR"

echo "[INFO] Building project..."
cmake --build "$BUILD_DIR"

echo "[INFO] Starting server..."
"$SERVER_BIN" &
SERVER_PID=$!

sleep 1

echo
echo "[INFO] Launching $NUM_CLIENTS concurrent HTTP clients..."

for ((i=1; i<=NUM_CLIENTS; i++)); do
(
    BODY="hello-world-$i"

    curl \
        --silent \
        --show-error \
        --max-time "$TIMEOUT" \
        --request POST \
        --header "Host: localhost" \
        --header "Content-Type: text/plain" \
        --header "Connection: close" \
        --data "$BODY" \
        --output "$TMP_DIR/client_${i}.body" \
        --write-out "%{http_code}" \
        "http://127.0.0.1:${PORT}/" \
        > "$TMP_DIR/client_${i}.status" \
        2> "$TMP_DIR/client_${i}.error"

) &

    CLIENT_PIDS+=($!)
done

echo "[INFO] All $NUM_CLIENTS clients launched."
echo "[INFO] Waiting for HTTP responses..."

# IMPORTANT:
# We wait for every curl process, but each curl has --max-time.
# Therefore this can NEVER wait indefinitely.
for pid in "${CLIENT_PIDS[@]}"; do
    wait "$pid" 2>/dev/null || true
done

echo
echo "[INFO] Verifying responses..."

FAIL=0

for ((i=1; i<=NUM_CLIENTS; i++)); do

    EXPECTED_BODY="pong"
    EXPECTED_STATUS="200"

    BODY_FILE="$TMP_DIR/client_${i}.body"
    STATUS_FILE="$TMP_DIR/client_${i}.status"
    ERROR_FILE="$TMP_DIR/client_${i}.error"

    if [ -f "$BODY_FILE" ]; then
        RESPONSE_BODY=$(cat "$BODY_FILE")
    else
        RESPONSE_BODY=""
    fi

    if [ -f "$STATUS_FILE" ]; then
        HTTP_STATUS=$(cat "$STATUS_FILE")
    else
        HTTP_STATUS=""
    fi

    if [ "$HTTP_STATUS" = "$EXPECTED_STATUS" ] &&
       [ "$RESPONSE_BODY" = "$EXPECTED_BODY" ]; then

        echo "[PASS] Client $i"

    else

        echo "[FAIL] Client $i"
        echo "       Expected status:  $EXPECTED_STATUS"
        echo "       Received status:  ${HTTP_STATUS:-<none>}"
        echo "       Expected body:    $EXPECTED_BODY"
        echo "       Received body:    ${RESPONSE_BODY:-<empty>}"

        if [ -s "$ERROR_FILE" ]; then
            echo "       curl error:"
            sed 's/^/       /' "$ERROR_FILE"
        fi

        FAIL=1
    fi

done

echo

if [ "$FAIL" -eq 0 ]; then
    echo "[PASS] All $NUM_CLIENTS concurrent HTTP clients received the correct response."
    exit 0
else
    echo "[FAIL] One or more clients did not receive the expected HTTP response."
    exit 1
fi