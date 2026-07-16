#include <non-blocking-server/Server.hpp>

int main() {
    Server* server = new Server();
    server->init();
    
    return 0;
}