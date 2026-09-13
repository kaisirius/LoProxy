#!/bin/bash
echo "Starting 3 backends..."
python3 -m http.server 3001 &
P1=$!
python3 -m http.server 3002 &
P2=$!
python3 -m http.server 3003 &
P3=$!

echo "Backends running on 3001, 3002, 3003"
echo "PIDs: $P1 $P2 $P3"
echo "Press Ctrl+C to stop all backends"

trap "kill $P1 $P2 $P3" SIGINT SIGTERM
wait