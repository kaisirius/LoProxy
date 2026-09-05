#pragma once
#include <stdint.h>
#include <string>
#include <cstring>
#include <http/HttpParser.hpp>

class ConnectionState { // Proxy Session
    private:
        int32_t connectedSocketFD;
        int32_t upstreamFD;
        char readBuffer[1025]; 
        std::string clientBuffer;
        std::string upstreamBuffer;
        HttpParser httpParser;
        bool upstreamConnectedFlag = false;

    public:
        ConnectionState(const int32_t fd);
        char* getReadBuffer();
        std::string getUpstreamBuffer();
        int32_t getConnectedSocketFD();
        int32_t getUpstreamFD();
        std::string getClientBuffer();
        std::string getParsedMethod();
        std::string getParsedURI();
        std::string getParsedBody();
        std::unordered_map<std::string, std::string> getParsedHeaders();
        bool getUpstreamConnectedFlag();

        void setReadBuffer(const char buffer[1025]);
        void setUpstreamBuffer(const std::string data);
        void setClientBuffer(const std::string updatedData);
        void setUpstreamConnectedFlag(bool flag);
        void setUpstreamFD(int32_t upstreamFD);
        ParseResult parse();    
        int32_t connectUpstream(std::string& host, int32_t port);
        
};