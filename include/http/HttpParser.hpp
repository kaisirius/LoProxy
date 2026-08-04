#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <http/ParsedRequest.hpp>
#include <state/IParserState.hpp>
#include <state/ParseReqLineState.hpp>

class HttpParser {
    private:
        ParsedRequest parsedReq;
    public:
        std::unique_ptr<IParserState> currentState = std::make_unique<ParseReqLineState>();

        ParseResult parse(const std::string& data, int startIdx);
        void reset();

};