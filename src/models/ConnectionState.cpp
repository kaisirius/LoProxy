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

void ConnectionState::setReadData(const std::string updatedData) {
    readData = updatedData;
}

void ConnectionState::setReadBuffer(const char buffer[1025]) {
    strcpy(readBuffer, buffer);
}

void ConnectionState::setWriteData(const std::string data) {
    writeData = data;
}