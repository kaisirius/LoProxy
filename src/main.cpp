#include <non-blocking-server/Server.hpp>
#include <memory>

int main() {
    std::unique_ptr<Server> server = std::make_unique<Server>();
    server.get()->init();
    server.get()->runEventLoop();
    
    return 0;
}