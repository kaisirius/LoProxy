#pragma once
#include <http/ParseResult.hpp>
#include <http/HttpParser.hpp>
#include <string>

class IParserState {
    protected:
        std::string streamedData = ""; 
    public:
        virtual ParseResult handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser) = 0;
        virtual bool isExtractable() = 0;
        virtual void extractFromStreamedData() = 0;
};