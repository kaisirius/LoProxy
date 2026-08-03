#include <state/IParserState.hpp>

class ParseHeaderState : public IParserState {
    ParseResult handleHTTPparsing(const std::string& data, size_t bytes, int startIdx);
};