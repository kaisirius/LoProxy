# Architecture

## Overview

LoProxy is a single-threaded, event-driven reverse proxy. A single OS thread runs an `epoll` event loop that handles all client connections, upstream connections, and I/O: no thread-per-connection model. A second background thread runs the health checker independently.

---

## Diagram

> The diagram below shows the full component layout and request flow through LoProxy.

```
┌─────────┐     TCP :8080      ┌─────────────────────────────────────────────────┐
│  Client │ ──────────────────▶│                    LoProxy                      │
└─────────┘                    │                                                 │
     ▲                         │   ┌──────────────────────────────────────────┐  │
     │                         │   │            epoll event loop              │  │
     │   response              │   │   accept() / EPOLLIN / EPOLLOUT events   │  │
     └─────────────────────────│   └──────────┬────────────────┬──────────────┘  │
                               │              │                │                 │
                               │   ┌──────────▼──────┐  ┌──────▼──────────┐      │
                               │   │   HttpParser    │  │  LoadBalancer   │      │
                               │   │  (state machine)│  │  RR / LeastConn │      │
                               │   └──────────┬──────┘  └──────┬──────────┘      │
                               │              │                │                 │
                               │   ┌──────────▼────────────────▼─────────────┐   │
                               │   │              ConnectionState            │   │
                               │   │    client FD + upstream FD + buffers    │   │
                               │   └───────────────────────┬─────────────────┘   │
                               │                           │  TCP outbound       │
                               │   ┌───────────────────────┼───────────────────┐ │
                               │   │   HealthChecker       │  (background      │ │
                               │   │   std::thread         │   thread probes)  │ │
                               │   └───────────────────────┼───────────────────┘ │
                               └───────────────────────────┼─────────────────────┘
                                                           │
                                      ┌────────────────────┼──────────────────┐
                                      │          Upstream Backends            │
                                      │   :3001        :3002         :3003    │
                                      └───────────────────────────────────────┘
```

---

## Component Breakdown

### `Server`

The central coordinator. Owns the listening socket, the `epoll` instance via `EpollEngine`, the session map (`connectedSockets`), the `clientFDs` set, the `LoadBalancer`, and the `HealthChecker`.

Responsibilities:

- Calls `EpollEngine::fetchReadySockets()` in a tight loop
- Dispatches each ready event to `handleAcceptEvent`, `handleReadEvent`, `handleUpstreamReadEvent` based on which FD fired and what event type arrived
- Calls `lb->selectBackend()` on each new connection to pick an upstream
- Calls `lb->onConnectionClosed()` when a session is torn down
- Owns teardown via `shutdownConnection(fd)`

### `EpollEngine`

Thin wrapper around the Linux `epoll` API. Exposes:

- `addObserver(fd)` : registers a new FD with default events (`EPOLLIN | EPOLLERR | EPOLLHUP`)
- `modifyObserver(fd, events)` : updates the event mask (used to toggle `EPOLLOUT` on/off)
- `removeObserver(fd)` : deregisters an FD
- `fetchReadySockets(timeoutMs)` : calls `epoll_wait`, returns ready events

### `HttpParser`

An incremental state machine HTTP/1.1 request parser. Fed bytes from `recv()` calls via `parse(data, startIdx)`. Internally transitions through states:

```
ParseReqLineState -> ParseHeaderState -> ParseBodyState -> ParseCompleteState
```

Each state accumulates bytes into `streamedData`, validates via regex when a terminator is found (`\r\n` for request line and each header, `\r\n\r\n` for end of headers), and extracts fields into `ParsedRequest`. Returns `INCOMPLETE`, `COMPLETE`, or `ERROR` so the caller knows whether to wait for more data or act.

### `LoadBalancer` / `RoundRobin` / `LeastConn`

Abstract base class with a pure virtual `selectBackend()`. Two concrete strategies:

- `RoundRobin` : maintains a `uint64_t counter`, selects `backends[(counter + i) % size]` where `i` skips unhealthy backends, advances counter past the selected index
- `LeastConn` : iterates all healthy backends, picks the one with minimum `activeConnections`, increments its count on select, decrements via `onConnectionClosed()`

### `ConnectionState`

Per-session state object. One instance per client connection, shared between the client FD map entry and the upstream FD map entry (via `shared_ptr`). Holds:

- `connectedSocketFD` : the client-side FD
- `upstreamFD` : the backend-side FD (`-1` until connected, reset to `-1` on FIN)
- `clientBuffer` : raw HTTP request bytes accumulating from client `recv()` calls
- `upstreamBuffer` : raw HTTP response bytes accumulating from upstream `recv()` calls
- `readBuffer` : 1025-byte scratch buffer for `recv()`
- `upstreamConnectedFlag` : whether the non-blocking `connect()` has completed
- `upstream` : pointer to the selected `UpstreamServer` (for `onConnectionClosed` calls)
- `HttpParser` : owned parser instance, reset on session teardown

### `HealthChecker`

Runs on a `std::thread` launched at startup. Every `interval` seconds, iterates all `UpstreamServer*` entries and calls `checkOne(server)`:

1. Creates a non-blocking socket
2. Calls `connect()` to the upstream host:port
3. If `EINPROGRESS`: waits up to 3 seconds with `poll()` for `POLLOUT`, then calls `getsockopt(SO_ERROR)` to confirm connection
4. Sets `server->isHealthy = true` on success, `false` on timeout or error
5. Closes the probe socket

Uses `std::condition_variable::wait_for` for the sleep so `stop()` can interrupt it immediately on shutdown.

### `Config`

Static loader. Reads `config/config.json` via `nlohmann::json`, populates a `Config` struct containing `listeningHost`, `listeningPort`, `lbStrategy`, and a `vector<UpstreamConfig>`. Throws typed `nlohmann::json` exceptions on malformed input, which `main()` catches and prints cleanly.

---

## Request Lifecycle

```
1. Client SYN arrives on :8080
   -> epoll fires EPOLLIN on listening socket
   -> accept() in a loop until EAGAIN
   -> ConnectionState created, clientFD registered in epoll
   -> connectUpstream() called: socket() + fcntl(O_NONBLOCK) + connect()
      -> returns EINPROGRESS (non-blocking)
   -> upstreamFD registered in epoll with EPOLLOUT

2. Client sends HTTP request bytes
   -> epoll fires EPOLLIN on clientFD
   -> recv() loop accumulates into ConnectionState::clientBuffer until EAGAIN
   -> HttpParser::parse() called on clientBuffer
      -> INCOMPLETE: wait for more data
      -> ERROR: send 400, teardown
      -> COMPLETE: modify upstreamFD to EPOLLOUT to flush buffered request

3. Upstream connection confirms (EPOLLOUT on upstreamFD, first time)
   -> getsockopt(SO_ERROR) == 0 confirms connection
   -> upstreamConnectedFlag = true
   -> clientBuffer forwarded via send() to upstreamFD
   -> upstreamFD modified back to EPOLLIN | EPOLLERR | EPOLLHUP

4. Upstream sends response
   -> epoll fires EPOLLIN on upstreamFD
   -> recv() loop accumulates into ConnectionState::upstreamBuffer until EAGAIN or FIN
   -> On FIN (recv returns 0):
      -> upstreamFD closed, removed from epoll and session map
      -> upstreamFD set to -1 on ConnectionState
      -> clientFD modified to EPOLLOUT

5. Response forwarded to client (EPOLLOUT on clientFD)
   -> send() loop flushes upstreamBuffer to clientFD
   -> clientFD modified back to EPOLLIN
   -> buffers cleared, session ready for next request

6. Client disconnects (recv returns 0 or EPOLLERR/EPOLLHUP on clientFD)
   -> shutdownConnection(clientFD) called
   -> lb->onConnectionClosed(upstream) decrements activeConnections
   -> both FDs closed and removed from epoll + session maps
```

---

## Thread Model

```
Main thread (event loop)          Health checker thread
       |                                   |
  epoll_wait()                      while(!stop_)
       |                              checkOne(backend1)
  handle events                      checkOne(backend2)
       |                              checkOne(backend3)
  recv / send                         cv.wait_for(interval)
       |                                   |
  reads upstream->isHealthy         writes upstream->isHealthy
  reads upstream->activeConnections  (std::atomic<bool>)
  writes upstream->activeConnections
  (single-threaded, no sync needed)
```

The only shared mutable state between threads is `UpstreamServer::isHealthy`, which is `std::atomic<bool>`. All other `UpstreamServer` fields (`activeConnections`, `host`, `port`) are exclusively owned by the event loop thread.

---

## Data Flow Summary

```
Client FD  ──recv──▶  clientBuffer  ──HttpParser──▶  COMPLETE
                                                          │
                                                    selectBackend()
                                                          │
upstreamFD ◀──send──  clientBuffer  ◀─────────────  upstream*
    │
    └──recv──▶  upstreamBuffer  ──send──▶  Client FD
```