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
    // storing config - IPv4 address container
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
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

    fileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

     if(fileDescriptor == -1) {
        std::cerr << "[ERROR]: " << "Cannot open socket at this port." << "\n";
        std::string err = strerror(errno);
        throw std::runtime_error(err);
    }

    int opt = 1;
    int flag = setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // flag should be 0 on success 
    if(flag == -1) {
        std::cerr << "[ERROR]: " << "Socket configuration failed to set." << "\n";
        throw std::runtime_error(strerror(errno));
    }

    socklen_t addrLen = sizeof(addr);

    if(bind(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), addrLen) == -1) {
        std::cerr << "[ERROR]: " << "Binding error." << "\n";
        throw std::runtime_error(strerror(errno));
        // EADDRINUSE : address already in use error comes here, solution -> SO_RESUSEADDR SO_REUSEPORT
    }
    if(listen(fileDescriptor, 5) == -1) {
        // if more than 5 connections come in OS networking queue for this socket, will show ECONNREFUSED
        std::cerr << "[ERROR]: " << "Cannot start listening on this port." << "\n";
        throw std::runtime_error(strerror(errno));
    }
    
    std::cout << "Server started listening on port 8080, Ready to accept connections." << "\n";



    while(true) {
        int connectedSocketFileDescriptor = accept(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), &addrLen);

        if(connectedSocketFileDescriptor == -1) {
            throw std::runtime_error(strerror(errno));
        }

        
        std::cout << "CLient connected: " << connectedSocketFileDescriptor << "\n";

        // receiveing & sending message
        char buffer[17];

        while(true) {
            ssize_t msgSizeRec = recv(connectedSocketFileDescriptor, &buffer, sizeof(buffer) - 1, 0);

            if(msgSizeRec == -1) {
                std::cerr << "[ERROR]: " << "Could not receive message." << "\n";
                throw std::runtime_error(strerror(errno));
            } else if(msgSizeRec == 0) {
                std::cout << "FIN received from client. Client disconnected." << "\n";
                break;
            }else {
                std::cout << "Size of message received: " << msgSizeRec << "\n";
                buffer[msgSizeRec] = '\0';
                std::cout << "Message: " << buffer;
            }

            sendAllBytes(connectedSocketFileDescriptor, msgSizeRec, buffer);
        }

        close(connectedSocketFileDescriptor);
    }
}

// echo "hello" | nc 127.0.0.1 8080

void Server::sendAllBytes(int connectedSocket, ssize_t bytesToSend, char buff[]) {
    ssize_t totalBytesSent = 0;
    while(totalBytesSent != bytesToSend) {
        int bytesSent = send(connectedSocket, buff + totalBytesSent, bytesToSend - totalBytesSent, 0);
        if(bytesSent == -1) {
            std::cerr << "[ERROR]: " << "Could not send message." << "\n";
            throw std::runtime_error(strerror(errno));
        } else {
            std::cout << "Size of message sent back: " << bytesSent << "\n";
        }

        totalBytesSent += bytesSent;
    }
} 


Server::~Server() {
    std::cout << "Closing server..." << "\n";
    close(fileDescriptor);
}