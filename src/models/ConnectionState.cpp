#include <models/ConnectionState.hpp>
#include <iostream>

ConnectionState::ConnectionState(const int32_t fd) {
    connectedSocketFD = fd;
    readData = "";
    writeData = "";
}

char* ConnectionState::getReadBuffer() {
    return readBuffer;
}

std::string ConnectionState::getWriteData() {
    return writeData;
}

int32_t ConnectionState::getConnectedSocketFD() {
    return connectedSocketFD;
}

std::string ConnectionState::getReadData() {
    return readData;
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

void ConnectionState::setReadData(const std::string updatedData) {
    readData = updatedData;
}

void ConnectionState::setReadBuffer(const char buffer[1025]) {
    strcpy(readBuffer, buffer);
}

void ConnectionState::setWriteData(const std::string data) {
    writeData = data;
}

// testing purpose
void ConnectionState::parseAndPrint() {
    httpParser.parse(readData, 0);
    std::cout <<"----- METHOD:" << httpParser.getParsedReqObj().method << "\n";
    std::cout <<"----- URI:" << httpParser.getParsedReqObj().uri << "\n";
    std::cout <<"----- VERSION:" << httpParser.getParsedReqObj().version << "\n";
    std::cout <<"----- HEADERS:" << "\n";
    for(auto it: httpParser.getParsedReqObj().headers) {
        std::cout << it.first << ": " << it.second << "\n";
    }
    std::cout <<"----- BODY:" << httpParser.getParsedReqObj().body << "\n";
}   