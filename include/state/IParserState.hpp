#pragma once
#include <http/ParseResult.hpp>
#include <string>

class HttpParser;

class IParserState {
    protected:
        std::string streamedData = ""; 
    public:
        size_t contentLength_ToParse = 0;
        ssize_t contentLength_Parsed = 0;
        virtual ParseResult handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser) = 0;
        virtual bool isExtractable() = 0;
        virtual void extractFromStreamedData(HttpParser* parser) = 0;
};