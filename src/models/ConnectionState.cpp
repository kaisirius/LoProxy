#include <models/ConnectionState.hpp>
#include <iostream>

ConnectionState::ConnectionState(const int32_t fd) {
    connectedSocketFD = fd;
    data = "";
}

char* ConnectionState::getReadBuffer() {
    return readBuffer;
}

char* ConnectionState::getWriteBuffer() {
    return writeBuffer;
}

int32_t ConnectionState::getConnectedSocketFD() {
    return connectedSocketFD;
}

std::string ConnectionState::getData() {
    return data;
}

void ConnectionState::setData(const std::string updatedData) {
    data = updatedData;
    std::cout << data;
}