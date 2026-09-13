#pragma once
#include <string>
#include <vector>

struct UpstreamConfig {
    std::string host;
    int32_t port;
};

struct Config {
    std::string listeningHost;
    int32_t listeningPort;
    std::string lbStrategy;
    std::vector<UpstreamConfig> upstreams;

    static Config load(const std::string& path); 
};