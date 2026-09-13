#pragma once
#include <stddef.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <unordered_set>
#include <models/proxy/ConnectionState.hpp>
#include <config/Config.hpp>
#include <lb/LoadBalancer.hpp>

class Server {
    private:
        int32_t fileDescriptor = -1; // -1 if system call fails else non negative integer returned on socket creation > 2 (0, 1, 2) are reserved for stdin, stdout and stderr
        sockaddr_in addr;
        socklen_t addrLen;
        std::unordered_set<int32_t> clientFDs;
        std::unordered_map<int32_t, std::shared_ptr<ConnectionState>> connectedSockets; // keyed with clientFD an upstreamFD
        LoadBalancer* lb = nullptr;
        std::vector<UpstreamServer*> upstreamServers;

        void sendAllBytes(int connectedSocket, ssize_t bytesToSend, const std::string &data);
        void handleEvent(const epoll_event event);
        void handleAcceptEvent();
        void handleReadEvent(const int32_t fd);
        void handleUpstreamReadEvent(const int32_t fd);
        void shutdownAllConnections();
        void shutdownConnection(const int fd);
        int32_t connectUpstream(std::string& host, int32_t port);
    public: 
        Server(Config config);
        void init();
        void runEventLoop();
        ~Server();
};