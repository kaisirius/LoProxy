#include <http/HttpParser.hpp>

ParseResult HttpParser::parse(const std::string& data, int startIdx) {
    return currentState.get()->handleHTTPparsing(data, startIdx, this);
}

void HttpParser::reset() {
    parsedReq.method = "";
    parsedReq.uri = "";
    parsedReq.body = "";
    parsedReq.version = "";
    parsedReq.headers.clear();
    parsedReq.complete = false;
}