#include <state/ParseCompleteState.hpp>

ParseResult ParseCompleteState::handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser) {
    // TO-DO
    ParseResult res(Status(COMPLETE), 0);
    return res;
}

bool ParseCompleteState::isExtractable() {
    return true;
}

void ParseCompleteState::extractFromStreamedData(HttpParser* parser) {

}