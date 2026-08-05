#include <state/ParseBodyState.hpp>

ParseResult ParseBodyState::handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser) {
    // TO-DO
    ParseResult res(Status(COMPLETE), 0);
    return res;
}

bool ParseBodyState::isExtractable() {
    return true;
}

void ParseBodyState::extractFromStreamedData(HttpParser* parser) {

}