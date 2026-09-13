#!/bin/bash
PASS=0
FAIL=0

check() {
    local desc=$1
    local expected=$2
    local actual=$3
    if echo "$actual" | grep -q "$expected"; then
        echo "PASS: $desc"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $desc"
        echo "  expected: $expected"
        echo "  got: $actual"
        FAIL=$((FAIL + 1))
    fi
}

# start backends
python3 -m http.server 3001 &> /dev/null &
B1=$!
python3 -m http.server 3002 &> /dev/null &
B2=$!
python3 -m http.server 3003 &> /dev/null &
B3=$!

# start proxy
./build/loproxy &> /dev/null &
PROXY=$!

sleep 4 # let everything start

# test 1: basic request succeeds
RES=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
check "basic request returns 200" "200" "$RES"

# test 2: multiple requests succeed
for i in {1..9}; do
    RES=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
    check "request $i returns 200" "200" "$RES"
done

# test 3: kill one backend, requests still succeed
kill $B2
sleep 3  # wait for health checker interval
RES=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
check "request succeeds after one backend dies" "200" "$RES"

# test 4: kill all backends, expect 503
kill $B1 $B3
sleep 3
RES=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
check "503 when all backends down" "503" "$RES"

# cleanup
kill $PROXY 2>/dev/null
wait 2>/dev/null

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] && exit 0 || exit 1