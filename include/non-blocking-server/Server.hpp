#pragma once
#include <stddef.h>
#include <netinet/in.h>

class Server {
    private:
        int32_t fileDescriptor = -1; // -1 if system call fails else non negative integer returned on socket creation > 2 (0, 1, 2) are reserved for stdin, stdout and stderr
        sockaddr_in addr;
        void sendAllBytes(int connectedSocket, ssize_t bytesToSend, char buff[]);

    public: 
        Server();
        void init();
        ~Server();
};