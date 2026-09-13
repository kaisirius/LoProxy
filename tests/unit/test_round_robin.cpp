#include <catch2/catch_test_macros.hpp>
#include <lb/LoadBalancer.hpp>
#include <lb/RoundRobin.hpp>

TEST_CASE("All 3 backends healthy, RR schedules 6 calls -> 2 call per backend") {
    std::string domain = "127.0.0.1";
    std::vector<UpstreamServer*> upstreams(3);
    upstreams[0] = new UpstreamServer();
    upstreams[0]->host = domain; upstreams[0]->port = 3001;
    upstreams[1] = new UpstreamServer();
    upstreams[1]->host = domain; upstreams[1]->port = 3002;
    upstreams[2] = new UpstreamServer();
    upstreams[2]->host = domain; upstreams[2]->port = 3003;

    LoadBalancer* lb = new RoundRobin(upstreams); 
    int callsToServer1 = 0, callsToServer2 = 0, callsToServer3 = 0;
    for(int i = 1; i <= 6; i++) {
        UpstreamServer* routedUpstream = lb->selectBackend();
    
        if(routedUpstream->port == 3001) callsToServer1++;
        else if(routedUpstream->port == 3002) callsToServer2++;
        else if(routedUpstream->port == 3003) callsToServer3++;
    }

    delete lb;

    REQUIRE(callsToServer1 == 2);
    REQUIRE(callsToServer2 == 2);
    REQUIRE(callsToServer3 == 2);
}


TEST_CASE("2 backends healthy, one fails in between. 9 calls -> first 3 when all 3 were healthy, remaining 6 gets distributed to remaining two backends") {
    std::string domain = "127.0.0.1";
    std::vector<UpstreamServer*> upstreams(3);
    upstreams[0] = new UpstreamServer();
    upstreams[0]->host = domain; upstreams[0]->port = 3001;
    upstreams[1] = new UpstreamServer();
    upstreams[1]->host = domain; upstreams[1]->port = 3002;
    upstreams[2] = new UpstreamServer();
    upstreams[2]->host = domain; upstreams[2]->port = 3003;

    LoadBalancer* lb = new RoundRobin(upstreams); 

    int callsToServer1 = 0, callsToServer2 = 0, callsToServer3 = 0;
    for(int i = 1; i <= 9; i++) {
        if(i == 4) {
            lb->getUpstreamBackends()[1]->isHealthy = false; // 2nd backend now marked unhealthy
        }
        UpstreamServer* routedUpstream = lb->selectBackend();
        if(routedUpstream->port == 3001) callsToServer1++;
        else if(routedUpstream->port == 3002) callsToServer2++;
        else if(routedUpstream->port == 3003) callsToServer3++;
    }
    delete lb;
    
    REQUIRE(callsToServer1 == 4);
    REQUIRE(callsToServer2 == 1);
    REQUIRE(callsToServer3 == 4);
}


