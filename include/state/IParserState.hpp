#pragma once
#include <http/ParseResult.hpp>
#include <string>

class IParserState {
    public:
        virtual ParseResult handleHTTPparsing(const std::string& data, size_t bytes, int startIdx) = 0;
};