#pragma once

#include <lb/LoadBalancer.hpp>

class LeastConn: public LoadBalancer {
public:
    uint64_t counter = 0;

    UpstreamServer* selectBackend() override;
    void onConnectionClosed(UpstreamServer* server) override;
    LeastConn(std::vector<UpstreamServer*> backends);
    ~LeastConn();
};