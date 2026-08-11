#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <http/ParsedRequest.hpp>
#include <state/ParseReqLineState.hpp>

class HttpParser {
    private:
        ParsedRequest parsedReq;
    public:
        std::unique_ptr<IParserState> currentState = std::make_unique<ParseReqLineState>();

        ParseResult parse(const std::string& data, int startIdx);
        ParsedRequest getParsedReqObj();
        void setParsedReqMethod(std::string _method);
        void setParsedReqURI(std::string _uri);
        void setParsedReqHeader(std::string key, std::string value);
        void setParsedReqBody(std::string body);
        void reset();

};