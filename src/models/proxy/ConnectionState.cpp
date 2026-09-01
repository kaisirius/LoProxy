#include <models/proxy/ConnectionState.hpp>
#include <iostream>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

ConnectionState::ConnectionState(const int32_t fd) {
    connectedSocketFD = fd;
    clientBuffer = "";
    upstreamBuffer = "";
}

char* ConnectionState::getReadBuffer() {
    return readBuffer;
}

std::string ConnectionState::getUpstreamBuffer() {
    return upstreamBuffer;
}

int32_t ConnectionState::getConnectedSocketFD() {
    return connectedSocketFD;
}

int32_t ConnectionState::getUpstreamFD() {
    return upstreamFD;
}

std::string ConnectionState::getClientBuffer() {
    return clientBuffer;
}

std::string ConnectionState::getParsedMethod() {
    return httpParser.getParsedReqObj().method;
}

std::string ConnectionState::getParsedURI() {
    return httpParser.getParsedReqObj().uri;
}

std::string ConnectionState::getParsedBody() {
    return httpParser.getParsedReqObj().body;
}

std::unordered_map<std::string, std::string> ConnectionState::getParsedHeaders() {
   return httpParser.getParsedReqObj().headers; 
}

void ConnectionState::setClientBuffer(const std::string updatedData) {
    clientBuffer = updatedData;
}

void ConnectionState::setReadBuffer(const char buffer[1025]) {
    strcpy(readBuffer, buffer);
}

void ConnectionState::setUpstreamBuffer(const std::string data) {
    upstreamBuffer = data;
}

void ConnectionState::setUpstreamConnectedFlag(bool flag) {
    upstreamConnectedFlag = flag;
}

ParseResult ConnectionState::parse() {
    return httpParser.parse(clientBuffer, 0);
}   

int32_t ConnectionState::connectUpstream(std::string &host, int32_t port) {
    std::cout << "[LOG]: Creating upstream socket to connect with backend" << "\n";

    upstreamFD = socket(AF_INET, SOCK_STREAM, 0);
    if(upstreamFD == -1) {
        std::cout << "[ERROR]: Internal server error while creating upstream socket." << "\n";
    } else {
        fcntl(upstreamFD, F_SETFL, O_NONBLOCK); // non blocking ops
        //method - 1 (handles both type of hosts 127.0.0.1 & localhost/xyz) 
        addrinfo* result;
        int status = getaddrinfo(host.c_str(), std::to_string(port).c_str(), nullptr, &result);

        // method - 2 (handles only hosts like 127.0.0.1)
        // sockaddr_in addr;
        // socklen_t addrLen;
        // addr.sin_family = AF_INET;
        // addr.sin_port = htons(8080);
        // inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if(status != 0) { // Address resolution error
            close(upstreamFD); // will close the FD since we can't resolve our connection and still return -1 as system failure
            std::cout << "[ERROR]: DNS Resolution failed." << "\n";
            return -1;
        }
        int upstreamConnectionStatus = connect(upstreamFD, result->ai_addr, result->ai_addrlen);
        
        if(upstreamConnectionStatus == -1 && errno != EINPROGRESS) {
            close(upstreamFD); // will close the FD since we can't resolve our connection and still return -1 as system failure
            std::cout << "[ERROR]: Connection to upstream failed." << "\n";
            freeaddrinfo(result);
            return -1;
        }
        freeaddrinfo(result);
    }

    return upstreamFD; // if returned -1 will catch it in server's event loop
}