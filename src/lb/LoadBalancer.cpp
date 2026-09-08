#include <lb/LoadBalancer.hpp>

LoadBalancer::LoadBalancer(std::vector<UpstreamServer> &backends): backends(backends) {}