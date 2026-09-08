#pragma once
#include <string>
#include <atomic>

struct UpstreamServer {
    std::string host;
    int32_t port;
    std::atomic<bool> isHealthy = true;
    int activeConnections = 0;
};