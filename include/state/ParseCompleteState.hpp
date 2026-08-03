#include <state/IParserState.hpp>

class ParseCompleteState : public IParserState {
    ParseResult handleHTTPparsing(const std::string& data, size_t bytes, int startIdx);
};