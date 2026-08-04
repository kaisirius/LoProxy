#include <state/IParserState.hpp>

class ParseReqLineState : public IParserState {
    ParseResult handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser);
    bool isExtractable();
    void extractFromStreamedData();
};