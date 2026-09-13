// scratch/fast_backend.cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <string>

int main(int argc, char* argv[]) {
    int port = atoi(argv[1]);
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    bind(fd, (sockaddr*)&addr, sizeof(addr));
    listen(fd, 1024);
    
    std::string response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 2\r\n"
        "Connection: close\r\n"
        "\r\n"
        "OK";

    while(true) {
        int client = accept(fd, nullptr, nullptr);
        if(client == -1) continue;
        
        char buf[4096];
        recv(client, buf, sizeof(buf), 0);
        send(client, response.c_str(), response.length(), 0);
        close(client);
    }
    
    close(fd);
    return 0;
}