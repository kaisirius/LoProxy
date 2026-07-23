#pragma once
#include <stddef.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <models/ConnectionState.hpp>

class Server {
    private:
        int32_t fileDescriptor = -1; // -1 if system call fails else non negative integer returned on socket creation > 2 (0, 1, 2) are reserved for stdin, stdout and stderr
        sockaddr_in addr;
        socklen_t addrLen;
        std::unordered_map<int, ConnectionState> connectedSockets;

        void sendAllBytes(int connectedSocket, ssize_t bytesToSend, char buff[]);
        void handleEvent(const epoll_event event);
        void handleAcceptEvent();
        void handleReadEvent(const uint32_t fd);
        void shutdownAllConnections();
        void shutdownConnection(const int fd);
    public: 
        Server();
        void init();
        void runEventLoop();
        ~Server();
};