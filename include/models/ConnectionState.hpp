#pragma once
#include <stdint.h>
#include <string>
#include <cstring>

class ConnectionState {
    private:
        int32_t connectedSocketFD;
        char readBuffer[1025]; // restricting buffer size to avoid starvation for other sockets
        char writeBuffer[1025];
        std::string data;
    public:
        ConnectionState(const int32_t fd);
        char* getReadBuffer();
        char* getWriteBuffer();
        int32_t getConnectedSocketFD();
        std::string getData();
        void setReadBuffer(const char buffer[1025]);
        void setWriteBuffer(const char buffer[1025]);
        void setData(const std::string updatedData);
};