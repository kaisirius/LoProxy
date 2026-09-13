#pragma once
#include <models/lb/UpstreamServer.hpp>
#include <vector>

class LoadBalancer {
public:
    virtual ~LoadBalancer() = default;
    virtual UpstreamServer* selectBackend() = 0;
    virtual void onConnectionClosed(UpstreamServer* server) {}
    std::vector<UpstreamServer*> getUpstreamBackends() {
        return backends;
    }
protected:
    std::vector<UpstreamServer*> backends;
    LoadBalancer(std::vector<UpstreamServer*> backends);
};