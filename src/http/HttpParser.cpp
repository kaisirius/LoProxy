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

    currentState = std::make_unique<ParseReqLineState>();
}

ParsedRequest HttpParser::getParsedReqObj() {
    return parsedReq;
}

void HttpParser::setParsedReqMethod(std::string _method) {
    this->parsedReq.method = _method;
}

void HttpParser::setParsedReqURI(std::string _uri) {
    this->parsedReq.uri = _uri;
}

void HttpParser::setParsedReqHeader(std::string key, std::string value) {
    this->parsedReq.headers[key] = value;
}

void HttpParser::setParsedReqBody(std::string body) {
    this->parsedReq.body = body;
}