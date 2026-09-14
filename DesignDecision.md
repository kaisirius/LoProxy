# Design Decisions

A record of non-obvious choices made during the build, and the reasoning behind each.

---

## 1. epoll over select/poll

`select` and `poll` both require passing the full set of watched file descriptors on every call. The kernel scans the entire set to find which FDs are ready: O(n) per call. Under load with hundreds of simultaneous connections, this becomes a bottleneck.

`epoll` uses a kernel-side data structure. `epoll_wait` returns only the FDs that are actually ready: O(active events) not O(watched FDs). For a proxy that may hold many idle connections open simultaneously, this scales significantly better.

`kqueue` (BSD/macOS) is functionally equivalent but Linux-only `epoll` was chosen since the project targets Linux exclusively and there was no portability requirement.

---

## 2. Level-triggered over edge-triggered epoll

`epoll` supports two notification modes:

- Level-triggered (default): epoll notifies you as long as the condition is true. If there are bytes to read and you don't read them all, epoll notifies you again next iteration.
- Edge-triggered (`EPOLLET`): epoll notifies you only when the state changes. If bytes arrive and you don't drain the socket completely, you never get notified again until more bytes arrive.

Edge-triggered is slightly more efficient but requires draining the FD completely on every notification: the `recv()` loop must run until `EAGAIN` without exception. A single missed `EAGAIN` check or early exit causes silent data loss that is extremely difficult to debug.

Level-triggered was chosen because correctness is easier to reason about. The performance difference at this scale is not meaningful, and the debugging cost of getting edge-triggered wrong is high.

---

## 3. Incremental state machine HTTP parser over a whole-message regex

TCP does not respect HTTP message boundaries. A single `recv()` call may return:

- Half a request line
- Three complete headers and the start of a fourth
- Two complete requests back to back

A regex applied to a complete string requires the full message to be present before parsing can begin. An incremental state machine processes bytes as they arrive, transitions state only when it finds a terminator (`\r\n`, `\r\n\r\n`), and returns `INCOMPLETE` when more bytes are needed.

The current implementation accumulates bytes in `streamedData` and then validates with a regex: this is a hybrid approach. A true byte-at-a-time state machine (as used in nginx and llhttp) would be more efficient but significantly more code. The hybrid approach is correct and sufficient for this project's scope.

---

## 4. Two FD entries in `connectedSockets` pointing to the same session

When a client connects, two entries are added to the session map:

```cpp
connectedSockets.insert({clientFD, socketState});
connectedSockets.insert({upstreamFD, socketState});
```

Both point to the same `shared_ptr<ConnectionState>`. This means `handleEvent` can look up the session from either FD without knowing in advance which side fired. The `clientFDs` set is used to distinguish which role a given FD plays.

The alternative was a separate `upstreamSockets` map. Two maps were more verbose: every lookup required checking both. The single map with a role discriminator was simpler at the call site.

The tradeoff: teardown must erase both keys explicitly. `shutdownConnection` always erases `clientFD`, `upstreamFD`, and both entries from `connectedSockets`. Missing either erase causes a dangling map entry that fires on a closed FD.

---

## 5. Non-blocking connect with EINPROGRESS / getsockopt pattern

`connect()` on a non-blocking socket returns `-1` with `errno == EINPROGRESS` immediately: the TCP handshake completes asynchronously. The kernel signals completion by making the socket writable (`EPOLLOUT`).

On the first `EPOLLOUT` event on the upstream FD, the proxy calls:

```cpp
getsockopt(upstreamFD, SOL_SOCKET, SO_ERROR, &error, &errorlen);
```

`error == 0` means the connection succeeded. Any other value means it failed (e.g. `ECONNREFUSED` if the backend is down). This is the POSIX-correct way to determine non-blocking connect outcome.

This pattern is more complex than blocking `connect()` but is necessary to avoid stalling the event loop during the TCP handshake: which can take hundreds of milliseconds to a remote host.

---

## 6. UpstreamServer as heap-allocated pointers, not value objects in a vector

`UpstreamServer` contains `std::atomic<bool> isHealthy` and `std::atomic<int> activeConnections`. `std::atomic` is neither copyable nor movable, which means `std::vector<UpstreamServer>` cannot reallocate its internal buffer (reallocation requires moving or copying elements).

The options were:

- `std::vector<UpstreamServer>` with `reserve()` to prevent reallocation: fragile, breaks silently if `push_back` is called after capacity is full
- `std::vector<UpstreamServer*>` with heap-allocated objects: robust, no copy or move ever required

Heap allocation was chosen. The `Server` destructor explicitly deletes each pointer. `LoadBalancer` and `HealthChecker` hold raw pointers into this array: they do not own the objects.

---

## 7. Health checker uses poll() for EINPROGRESS instead of epoll

The health checker runs its own probe connections synchronously on the background thread. It creates a non-blocking socket, calls `connect()`, and if `EINPROGRESS` is returned, waits with `poll()` up to 3 seconds for `POLLOUT`.

Using the main epoll instance from a background thread would require locking or careful coordination. Using a per-probe `poll()` call on the background thread is simpler and correct: the health checker has no other work to do while waiting for a probe to complete.

The 3-second `poll` timeout defines the maximum time a health probe can block the checker thread per backend. With 3 backends, worst-case probe time is 9 seconds per interval cycle.

---

## 8. Connection closure waits for upstream FIN rather than tracking Content-Length

When the upstream sends a response, the proxy accumulates bytes until `recv()` returns 0 (upstream closed the connection). Only then is the accumulated response forwarded to the client.

The alternative is parsing the upstream's `Content-Length` header and forwarding bytes as they arrive (streaming). Streaming is more efficient for large responses: it reduces end-to-end latency and does not require buffering the full response in memory.

The FIN-based approach was chosen for simplicity:

- No need to parse the upstream response headers
- No need to handle chunked transfer encoding
- Works correctly for all response types

The limitation is that large responses (e.g. file downloads) are fully buffered in memory before being sent to the client. For a learning project serving typical API responses this is acceptable.

---

## 9. `shared_ptr` for ConnectionState

`ConnectionState` is referenced from two places in `connectedSockets` (client FD key and upstream FD key). Using a `shared_ptr` means whichever entry is erased last automatically manages the lifetime: no manual tracking of which side was erased first.

Raw pointers would require tracking whether the other map entry has already been erased before deleting. `shared_ptr` makes this automatic at the cost of a small reference count overhead per session.

---

## 10. Synchronous send() in the hot path

`sendAllBytes()` loops until all bytes are sent:

```cpp
while(totalBytesSent != bytesToSend) {
    send(connectedSocket, data + totalBytesSent, remaining, 0);
}
```

This blocks the event loop thread until the kernel's send buffer has accepted all the data. Under high concurrency with a slow client, this can stall all other connections while waiting for a single client's TCP window to open.

The correct solution is non-blocking buffered writes: register `EPOLLOUT` on the client FD, buffer any bytes that `send()` could not immediately accept, and drain the buffer on subsequent `EPOLLOUT` events. This is how nginx achieves high throughput.

The synchronous approach was chosen to keep the write path simple while learning the event loop pattern. The benchmark section of the README documents this as the measured bottleneck, along with the architectural fix.