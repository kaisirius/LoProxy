#pragma once
#include <stdint.h>
#include <string>
#include <cstring>
#include <http/HttpParser.hpp>

class ConnectionState {
    private:
        int32_t connectedSocketFD;
        char readBuffer[1025]; 
        std::string readData;
        std::string writeData;
        HttpParser httpParser;
        
    public:
        ConnectionState(const int32_t fd);
        char* getReadBuffer();
        std::string getWriteData();
        int32_t getConnectedSocketFD();
        std::string getReadData();
        std::string getParsedMethod();
        std::string getParsedURI();
        std::string getParsedBody();
        std::unordered_map<std::string, std::string> getParsedHeaders();

        void setReadBuffer(const char buffer[1025]);
        void setWriteData(const std::string data);
        void setReadData(const std::string updatedData);

        // for testing purpose (partially created parser)
        ParseResult parse();

};