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
        std::unique_ptr<IParserState> currentState = std::make_unique<ParseReqLineState>();
    
};