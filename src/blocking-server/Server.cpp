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

    fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

    // IPv4 address container
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        
    if(fileDescriptor == -1) {
        std::cout << "[ERROR]: " << "Cannot open socket at this port." << "\n";
        std::string err = strerror(errno);
        throw std::runtime_error(err);
    }
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

void Server::init() {
    std::cout << "Starting server at 127.0.0.1" << "\n";

    socklen_t addrLen = sizeof(addr);

    if(bind(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), addrLen) == -1) {
        std::cout << "[ERROR]: " << "Binding error." << "\n";
        throw std::runtime_error(strerror(errno));
    }
    if(listen(fileDescriptor, 5) == -1) {
        // if more than 5 connections come in OS networking queue for this socket, will show ECONNREFUSED
        std::cout << "[ERROR]: " << "Cannot start listening on this port." << "\n";
        throw std::runtime_error(strerror(errno));
    }
    
    std::cout << "Server started listening on port 8080, Ready to accept connections." << "\n";

    int connectedSocketFileDescriptor = accept(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), &addrLen);

    if(connectedSocketFileDescriptor == -1) {
        throw std::runtime_error(strerror(errno));
    }

    std::cout << "CLient connected: " << connectedSocketFileDescriptor << "\n";

    // receiveing message from client
    char* buffer = new char[16];
    ssize_t msgSizeRec = recv(connectedSocketFileDescriptor, buffer, 16, 0);

    if(msgSizeRec == -1) {
        std::cout << "[ERROR]: " << "Could not receive message." << "\n";
        throw std::runtime_error(strerror(errno));
    } else {
        std::cout << "Size of message received: " << msgSizeRec << "\n";
        if(msgSizeRec >= 0) {
            buffer[msgSizeRec] = '\0';
            std::cout << "Message: " << buffer;
        }
    }

    // sending message to client (as of now ping pong server)
    ssize_t msgSizeSent = send(connectedSocketFileDescriptor, buffer, 16, 0);

    if(msgSizeSent == -1) {
        std::cout << "[ERROR]: " << "Could not receive message." << "\n";
        throw std::runtime_error(strerror(errno));
    } else {
        std::cout << "Size of message sent back: " << msgSizeSent << "\n";
    }


    std::cout << "Closing server..." << "\n";
    close(fileDescriptor);
}

// echo "hello" | nc 127.0.0.1 8080

Server::~Server() {}