#include <lb/RoundRobin.hpp>

RoundRobin::RoundRobin(std::vector<UpstreamServer> &backends): LoadBalancer(backends) {}

UpstreamServer* RoundRobin::selectBackend() {
    int size = backends.size();
    for(int i = 0; i < size; i++) {
        UpstreamServer& upstream = backends[(counter + i) % size];
        if(upstream.isHealthy) {
            counter = (counter + i + 1) % size;
            return &upstream;
        }
    }
    return nullptr; // all backends unhealthy
}

RoundRobin::~RoundRobin() {}