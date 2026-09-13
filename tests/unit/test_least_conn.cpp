#include <catch2/catch_test_macros.hpp>
#include <lb/LoadBalancer.hpp>
#include <lb/LeastConn.hpp>

TEST_CASE("All 3 backends healthy, 2 backends have connnections already so request should go to 3rd upstream") {
    std::string domain = "127.0.0.1";
    std::vector<UpstreamServer*> upstreams(3);
    upstreams[0] = new UpstreamServer();
    upstreams[0]->host = domain; upstreams[0]->port = 3001; upstreams[0]->activeConnections = 2;
    upstreams[1] = new UpstreamServer();
    upstreams[1]->host = domain; upstreams[1]->port = 3002; upstreams[1]->activeConnections = 1;
    upstreams[2] = new UpstreamServer();
    upstreams[2]->host = domain; upstreams[2]->port = 3003;

    LoadBalancer* lb = new LeastConn(upstreams); 
    int callsToServer1 = 0, callsToServer2 = 0, callsToServer3 = 0;
    for(int i = 1; i <= 2; i++) {
        UpstreamServer* routedUpstream = lb->selectBackend();
        if(routedUpstream->port == 3001) callsToServer1++;
        else if(routedUpstream->port == 3002) callsToServer2++;
        else if(routedUpstream->port == 3003) callsToServer3++;
    }

    delete lb;

    REQUIRE(callsToServer1 == 0);
    REQUIRE(callsToServer2 == 1);
    REQUIRE(callsToServer3 == 1);
}


TEST_CASE("On connection closed correctly decrements the active connections") {
    std::string domain = "127.0.0.1";
    std::vector<UpstreamServer*> upstreams(1);
    upstreams[0] = new UpstreamServer();
    upstreams[0]->host = domain; upstreams[0]->port = 3001; 
    upstreams[0]->activeConnections = 1;

    LoadBalancer* lb = new LeastConn(upstreams); 

    UpstreamServer* selectedServer = lb->selectBackend(); // active connections = 2
    REQUIRE(upstreams[0]->activeConnections == 2);

    lb->onConnectionClosed(selectedServer);
    
    delete lb;
    
    REQUIRE(upstreams[0]->activeConnections == 1);
}


