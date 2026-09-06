#pragma once
#include <string>
#include <vector>

struct UpstreamServer {
    std::string host;
    int32_t port;
    bool healthy = true;
};

struct Config {
    std::string listeningHost;
    int32_t listeningPort;
    std::string lbStrategy;
    std::vector<UpstreamServer> upstreams;

    static Config load(const std::string& path); 
};