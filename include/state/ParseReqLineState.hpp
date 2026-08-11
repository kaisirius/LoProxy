#pragma once
#include <state/IParserState.hpp>

class ParseReqLineState : public IParserState {
    public:
        ParseResult handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser);
        bool isExtractable() override;
        void extractFromStreamedData(HttpParser* parser) override;
};