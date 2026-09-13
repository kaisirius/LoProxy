#pragma once

#include <lb/LoadBalancer.hpp>

class RoundRobin: public LoadBalancer {
public:
    uint64_t counter = 0;

    UpstreamServer* selectBackend() override;
    RoundRobin(std::vector<UpstreamServer*> backends);
    ~RoundRobin();
};