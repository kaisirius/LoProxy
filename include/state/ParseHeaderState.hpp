#include <state/IParserState.hpp>

class ParseHeaderState : public IParserState {
    ParseResult handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser);
    bool isExtractable();
    void extractFromStreamedData();
};