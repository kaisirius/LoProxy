#include <non-blocking-server/Server.hpp>

int main() {
    Server* server = new Server();
    server->init();
    server->runEventLoop();
    
    return 0;
}