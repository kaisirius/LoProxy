#include <state/IParserState.hpp>

class ParseBodyState : public IParserState {
    ParseResult handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser);
    bool isExtractable();
    void extractFromStreamedData();
};