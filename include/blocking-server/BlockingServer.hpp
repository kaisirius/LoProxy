#pragma once
#include <memory>
#include <mutex>
#include <stddef.h>
#include <netinet/in.h>

class BlockingServer {
    private:
    static std::unique_ptr<BlockingServer> BlockingServerInstance;
        static std::mutex mtx;
        int32_t fileDescriptor = -1; // -1 if system call fails else non negative integer returned on socket creation > 2 (0, 1, 2) are reserved for stdin, stdout and stderr
        sockaddr_in addr;
        BlockingServer();
        void sendAllBytes(int connectedSocket, ssize_t bytesToSend, char buff[]);
    public: 
        BlockingServer(const BlockingServer& BlockingServer) = delete;
        BlockingServer& operator = (const BlockingServer& BlockingServer) = delete;
        static BlockingServer* getInstance();
        void init();
        
        ~BlockingServer();
};