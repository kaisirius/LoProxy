#include <blocking-server/Server.hpp>

int main() {
    Server* server = Server::getInstance();
    server->init();
    
    return 0;
}