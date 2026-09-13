#pragma once
#include <string>
#include <atomic>

struct UpstreamServer {
    std::string host;
    int32_t port;
    std::atomic<bool> isHealthy = true;
    std::atomic<int> activeConnections = 0;
};