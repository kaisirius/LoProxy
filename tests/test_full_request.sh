#!/usr/bin/env bash

set -e

BUILD_DIR="build"

echo "[INFO] Configuring project..."
cmake -S . -B "$BUILD_DIR"

echo "[INFO] Building HTTP parser test..."
cmake --build "$BUILD_DIR" --target test_http_parser

echo "[INFO] Running full request test..."

"$BUILD_DIR/test_http_parser" \
    "[full_request]"

echo "[PASS] Full request test passed."