#pragma once
#include <memory>
#include <mutex>
#include <stddef.h>
#include <netinet/in.h>

class Server {
    private:
        static std::unique_ptr<Server> serverInstance;
        static std::mutex mtx;
        int32_t fileDescriptor = -1; // -1 if system call fails else non negative integer returned on socket creation > 2 (0, 1, 2) are reserved for stdin, stdout and stderr
        sockaddr_in addr;
        Server();
        void sendAllBytes(int connectedSocket, ssize_t bytesToSend, char buff[]);
    public: 
        Server(const Server& server) = delete;
        Server& operator = (const Server& server) = delete;
        static Server* getInstance();
        void init();
        
        ~Server();
};