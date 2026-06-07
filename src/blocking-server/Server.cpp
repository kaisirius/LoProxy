#include <blocking-server/Server.hpp>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>

std::unique_ptr<Server> Server::serverInstance = nullptr;
std::mutex Server::mtx;

Server::Server() {
    std::cout << "Starting server at 127.0.0.1" << "\n";

    fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

    // IPv4 address container
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        
    if(fileDescriptor == -1) {
        std::string err = strerror(errno);
        throw std::runtime_error(err);
    }

    socklen_t addrLen = sizeof(addr);

    if(bind(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), addrLen) == -1) {
        throw std::runtime_error(strerror(errno));
    }
    if(listen(fileDescriptor, 5) == -1) {
        // if more than 5 connections come in OS networking queue for this socket, will show ECONNREFUSED
        throw std::runtime_error(strerror(errno));
    }
    
    int connectedSocketFileDescriptor = accept(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), &addrLen);

    if(connectedSocketFileDescriptor == -1) {
        throw std::runtime_error(strerror(errno));
    }

    std::cout << "CLient connected: " << connectedSocketFileDescriptor << "\n";

    std::cout << "Closing server..." << "\n";
    close(fileDescriptor);
}

Server* Server::getInstance() {
    if(serverInstance == nullptr) {
        std::lock_guard<std::mutex> lock(mtx);
        if(serverInstance == nullptr) {
            serverInstance = std::unique_ptr<Server>(new Server());
        }
    }
    return serverInstance.get();
}

Server::~Server() {}