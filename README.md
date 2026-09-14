# LoProxy

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.17+-064F8C?logo=cmake&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Linux-FCC624?logo=linux&logoColor=black)
![License](https://img.shields.io/badge/license-MIT-green)
![Build](https://img.shields.io/badge/build-passing-brightgreen)

A lightweight reverse proxy with load balancing, built from scratch in C++17 on Linux. LoProxy uses a non-blocking `epoll`-based event loop to handle multiple concurrent connections on a single thread, routes traffic across backend servers using configurable load balancing strategies, and actively monitors backend health to route around failures.

Built as a systems programming learning project to understand how reverse proxies work at the socket level.

---

## Features

- Non-blocking I/O event loop using `epoll` (level-triggered)
- HTTP/1.1 request parsing via an incremental state machine parser
- Reverse proxying : full request forwarding and response relay
- Load balancing strategies : Round Robin and Least Connections
- Active health checking : background thread probes backends at a configurable interval and automatically removes unhealthy upstreams from rotation
- Graceful error responses : `400 Bad Request`, `502 Bad Gateway`, `503 Service Unavailable`
- JSON-based configuration via `nlohmann/json`
- Unit tested with Catch2

---

## Architecture

```
Client
  |
  | TCP connection (port 8080)
  v
+---------------------------+
|        LoProxy            |
|                           |
|  epoll event loop         |
|    |                      |
|    +-- HttpParser         |  parses incoming request
|    |   (state machine)    |
|    |                      |
|    +-- LoadBalancer       |  selects upstream
|    |   RoundRobin /       |
|    |   LeastConnections   |
|    |                      |
|    +-- HealthChecker      |  background thread
|        (std::thread)      |  probes backends every N seconds
|                           |
+---------------------------+
  |
  | TCP connection (upstream port)
  v
Backend Server (3001 / 3002 / 3003)
```

**Request lifecycle:**

1. Client connects : `accept()` creates a client FD and registers it with epoll
2. Client sends HTTP request : `EPOLLIN` fires, bytes are fed incrementally into `HttpParser`
3. On `COMPLETE` : `LoadBalancer::selectBackend()` picks an upstream, a non-blocking `connect()` is initiated to it
4. On upstream `EPOLLOUT` (connection ready) : buffered request bytes are forwarded via `send()`
5. Upstream responds : `EPOLLIN` fires on the upstream FD, response bytes accumulate
6. On upstream FIN : accumulated response is forwarded to the client FD, connection is torn down

---

## Project Structure

```
LoProxy/
├── CMakeLists.txt
├── config/
│   └── config.json
├── include/
│   ├── non-blocking-server/
│   │   └── Server.hpp
│   ├── engine/
│   │   └── EpollEngine.hpp
│   ├── http/
│   │   ├── HttpParser.hpp
│   │   ├── HttpResponse.hpp
│   │   └── ParseResult.hpp
│   ├── state/
│   │   ├── IParserState.hpp
│   │   ├── ParseReqLineState.hpp
│   │   ├── ParseHeaderState.hpp
│   │   ├── ParseBodyState.hpp
│   │   └── ParseCompleteState.hpp
│   ├── models/
│   │   ├── proxy/
│   │   │   └── ConnectionState.hpp
│   │   └── lb/
│   │       ├── UpstreamServer.hpp
│   │       └── HealthChecker.hpp
│   ├── config/
│   │   └── Config.hpp
│   └── lb/
│       ├── LoadBalancer.hpp
│       ├── RoundRobin.hpp
│       └── LeastConn.hpp
├── src/                       # mirror of include/, implementation files
├── tests/
│   ├── unit/
│   │   ├── test_http_parser.cpp
│   │   ├── test_config_json.cpp
│   │   ├── test_round_robin.cpp
│   │   └── test_least_conn.cpp
│   ├── integration/
│   │   └── test_proxy_e2e.sh
│   └── scratch/
│       └── fast_backend.cpp
└── scripts/
    ├── run_backends.sh
    ├── benchmark_50_fast_backend.sh
    ├── benchmark_50.sh
    └── benchmark_100.sh
```

---

## Requirements

| Dependency | Version | Notes |
|---|---|---|
| GCC / Clang | GCC 11+ / Clang 13+ | C++17 required |
| CMake | 3.17+ | Build system |
| nlohmann/json | 3.12.0 | Fetched automatically via FetchContent |
| Catch2 | v3.8.1 | Fetched automatically via FetchContent |
| Python 3 | Any | Optional : used as a simple backend in integration tests |
| wrk | Any | Optional : HTTP benchmarking tool |

> LoProxy uses Linux-specific APIs (`epoll`, `SO_REUSEADDR`, POSIX sockets). It is not portable to macOS or Windows.

---

## Build

```bash
git clone https://github.com/kaisirius/LoProxy.git
cd LoProxy

cmake -B build
cmake --build build
```

This produces the following binaries inside `build/`:

| Binary | Description |
|---|---|
| `loproxy` | The proxy server |
| `test_http_parser` | HTTP parser unit tests |
| `test_config_json` | Config loader unit tests |
| `test_round_robin` | Round Robin LB unit tests |
| `test_least_conn` | Least Connections LB unit tests |
| `fast_backend` | Minimal C++ backend used for benchmarking |

---

## Configuration

The upstream list, load balancing strategy, and listen port are configured in `config/config.json`:

```json
{
  "listen_host": "127.0.0.1",
  "listen_port": 8080,
  "lb_strategy": "round_robin",
  "upstreams": [
    { "host": "127.0.0.1", "port": 3001 },
    { "host": "127.0.0.1", "port": 3002 },
    { "host": "127.0.0.1", "port": 3003 }
  ]
}
```

Supported values for `lb_strategy`:

- `round_robin` : distributes requests evenly across healthy backends in order
- `least_connections` : always routes to the backend with the fewest active connections

> Note : the config file path is currently hardcoded. To change the number of upstreams, update both `config.json` and start the corresponding number of backend servers.

---

## Running

**Step 1 : Start backend servers**

Using the provided script (starts 3 Python HTTP servers):
```bash
bash scripts/run_backends.sh
```

Or start the fast C++ backend for benchmarking:
```bash
./build/fast_backend 3001 &
./build/fast_backend 3002 &
./build/fast_backend 3003 &
```

**Step 2 : Start the proxy**

```bash
./build/loproxy
```

The proxy listens on `127.0.0.1:8080` by default.

**Step 3 : Send a request**

```bash
curl -v http://localhost:8080/
```

---

## Running Tests

**Unit tests:**

```bash
./build/test_http_parser
./build/test_config_json
./build/test_round_robin
./build/test_least_conn
```

**Integration tests** (requires Python 3):

```bash
bash tests/integration/test_proxy_e2e.sh
```

The integration test script:
- Starts 3 backend servers automatically
- Verifies successful proxying across all backends
- Kills one backend and verifies traffic continues to the remaining two
- Kills all backends and verifies a `503` is returned
- Cleans up all processes on exit

---

## Benchmarks

**Hardware:** 12th Gen Intel Core i5-12450H, 16GB RAM, Linux  
**Backends:** 3x `python backends`   
**Tool:** `wrk`

```
wrk -t4 -c50 -d30s http://localhost:8080/
```

| Scenario | Connections | Req/sec | Avg Latency | Max Latency |
|---|---|---|---|---|
| Python backends (proxied) | 50 | 1,877 | 21ms | 1.68s |
| No backends (503 path) | 100 | 53,216 | < 1ms | - |

**Interpreting these numbers:**

The 503 path (no upstream I/O) at 53k req/sec demonstrates the event loop itself has low overhead. The proxied throughput of ~1.8k req/sec is bounded by a synchronous `send()` in the hot path : when writing the upstream response back to the client, the event loop blocks until all bytes are flushed. Under high concurrency this creates head-of-line blocking where one slow client stalls all other connections.

The architectural fix is non-blocking buffered writes : register `EPOLLOUT` on the client FD, buffer unsent bytes, and drain the buffer incrementally across multiple event loop iterations. This is how production proxies like nginx achieve 50k+ req/sec on comparable hardware.

---

## Design Decisions

**Why `epoll` over `select`/`poll`:** `epoll` scales O(1) with the number of active events rather than O(n) with the number of watched file descriptors. For a proxy managing hundreds of simultaneous client and upstream FDs, this matters.

**Why level-triggered over edge-triggered:** Level-triggered is simpler to reason about correctly. Edge-triggered requires draining the FD completely on every notification or events are missed.

**Why a state machine HTTP parser over regex:** The parser must handle TCP fragmentation : a single `recv()` call may return a partial request line, partial headers, or even bytes from two requests back to back. A state machine accumulates bytes across multiple calls and transitions state only when a complete token is found. Regex over the full accumulated buffer works correctly but is less efficient for large requests.

**Why `std::atomic<bool>` for `isHealthy`:** The health checker runs on a background `std::thread` and writes `isHealthy`. The event loop thread reads it inside `selectBackend()`. A plain `bool` would be a data race. `std::atomic<bool>` makes the read-write pair safe without a mutex, since only one field is shared between the two threads.

---

## Known Limitations

- HTTP/1.1 keep-alive is not supported : connections are closed after each response
- No TLS/HTTPS support
- Synchronous `send()` in the write path limits throughput under high concurrency (see Benchmarks)
- Config file path is hardcoded at compile time and needs manual changes for more/less number of backends.
- No weighted load balancing strategy
- HTTP request body size is limited by the read buffer size

---

## License

MIT License. See [LICENSE](LICENSE) for details.

---

## Author

[Garvit Khurana](https://github.com/kaisirius)