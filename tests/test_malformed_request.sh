#!/usr/bin/env bash

set -e

BUILD_DIR="build"

echo "[INFO] Configuring project..."
cmake -S . -B "$BUILD_DIR"

echo "[INFO] Building HTTP parser test..."
cmake --build "$BUILD_DIR" --target test_http_parser

echo "[INFO] Running malformed request-line test..."

"$BUILD_DIR/test_http_parser" \
    "[malformed]"

echo "[PASS] Malformed request-line test passed."