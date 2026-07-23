#pragma once
#include <stdint.h>
#include <string>
#include <cstring>

class ConnectionState {
    private:
        int32_t connectedSocketFD;
        char readBuffer[5]; // restricting buffer size to avoid starvation for other sockets
        char writeBuffer[5];
        std::string data;
    public:
        ConnectionState(const int32_t fd);
        char* getReadBuffer();
        char* getWriteBuffer();
        int32_t getConnectedSocketFD();
        std::string getData();

        void setReadBuffer(char buffer[5]) {
            strcpy(readBuffer, buffer);
        }

        void setWriteBuffer(char buffer[1025]) {
            strcpy(writeBuffer, buffer);
        }

        void setData(const std::string updatedData);
};