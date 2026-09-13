    #include <lb/LeastConn.hpp>

    LeastConn::LeastConn(std::vector<UpstreamServer*> backends): LoadBalancer(backends) {}

    UpstreamServer* LeastConn::selectBackend() {
        int minConn = 1e9;
        UpstreamServer* upstream = nullptr;
        for(ssize_t i = 0; i < (ssize_t)backends.size(); i++) {
            if(backends[i]->isHealthy && backends[i]->activeConnections < minConn) {
                minConn = backends[i]->activeConnections;
                upstream = backends[i];
            }
        }
        if(upstream) upstream->activeConnections++;
        return upstream; // all backends unhealthy
    }

    void LeastConn::onConnectionClosed(UpstreamServer* server) {
        server->activeConnections = std::max(server->activeConnections.load() - 1, 0);
    }

    LeastConn::~LeastConn() {}