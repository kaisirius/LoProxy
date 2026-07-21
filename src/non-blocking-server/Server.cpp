#include <non-blocking-server/Server.hpp>
#include <engine/EpollEngine.hpp>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

Server::Server() {
    // storing config - IPv4 address container
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    socklen_t addrLen = sizeof(addr);
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

    EpollEngine* epollEngine = EpollEngine::getInstance();
    if(epollEngine) {
        epollEngine->addObserver(fileDescriptor);
        epollEngine->modifyObserver(fileDescriptor, EPOLLIN); // because server is just a listening socket
    } else {
        throw std::runtime_error("Server initialisation failed due to null epoll engine.");
    }

    // making listening socket non blocking so that accept() is non blocking op
    fcntl(fileDescriptor, F_SETFL, O_NONBLOCK);

    addrLen = sizeof(addr);

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
} 

void Server::runEventLoop() {

    std::vector<epoll_event> readyEvents;
    while(true) {
      readyEvents = EpollEngine::getInstance()->fetchReadySockets(fileDescriptor);  
      for(const epoll_event event: readyEvents) {
        handleEvent(event);
      }
    }
}
    //     // receiveing & sending message
    //     char buffer[17];

    //     while(true) {
    //         ssize_t msgSizeRec = recv(connectedSocketFD, &buffer, sizeof(buffer) - 1, 0);

    //         if(msgSizeRec == -1) {
    //             std::cerr << "[ERROR]: " << "Could not receive message." << "\n";
    //             throw std::runtime_error(strerror(errno));
    //         } else if(msgSizeRec == 0) {
    //             std::cout << "FIN received from client. Client disconnected." << "\n";
    //             break;
    //         }else {
    //             std::cout << "Size of message received: " << msgSizeRec << "\n";
    //             buffer[msgSizeRec] = '\0';
    //             std::cout << "Message: " << buffer;

    //         }

    //         sendAllBytes(connectedSocketFD, msgSizeRec, buffer);
    //     }

    //     close(connectedSocketFD);
    // }


// echo "hello" | nc 127.0.0.1 8080

void Server::handleEvent(const epoll_event event) {
    
    if(event.data.fd == fileDescriptor) {
        handleAcceptEvent();
    } else {
        // TODO
    }
}

void Server::handleAcceptEvent() {

    int connectedSocketFD = accept(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), &addrLen);;
    while(connectedSocketFD != -1) {
        fcntl(connectedSocketFD, F_SETFL, O_NONBLOCK);
        std::cout << "CLient connected: " << connectedSocketFD << "\n";

        connectedSocketFD = accept(fileDescriptor, reinterpret_cast<sockaddr*>(&addr), &addrLen);
    }
}

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