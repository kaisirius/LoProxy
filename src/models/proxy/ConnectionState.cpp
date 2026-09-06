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
    upstreamFD = -1;
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

bool ConnectionState::getUpstreamConnectedFlag() {
    return upstreamConnectedFlag;
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

void ConnectionState::setUpstreamFD(int32_t upstreamFD) {
    this->upstreamFD = upstreamFD;
}

ParseResult ConnectionState::parse() {
    return httpParser.parse(clientBuffer, 0);
}   