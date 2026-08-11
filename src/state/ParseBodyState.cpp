#include <state/ParseBodyState.hpp>
#include <http/HttpParser.hpp>
#include <state/ParseCompleteState.hpp>

ParseResult ParseBodyState::handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser) {
    ParseResult res(Status(COMPLETE), 0);

    int currentIdx = startIdx;
    while(contentLength_Parsed != contentLength_ToParse && currentIdx < (int)data.length()) {
        streamedData += data[currentIdx++];
        contentLength_Parsed++;
    }
    if(isExtractable()) { // unnecessary, just adding it for consistency in code design
        extractFromStreamedData(parser);

        parser->currentState = std::make_unique<ParseCompleteState>();

        ParseResult nextStateRes = parser->currentState.get()->handleHTTPparsing(data, currentIdx, parser); 
                
        res.status = nextStateRes.status;
        res.bytes_consumed += nextStateRes.bytes_consumed;
    }
    return res;
}

bool ParseBodyState::isExtractable() {
    return true;
}

void ParseBodyState::extractFromStreamedData(HttpParser* parser) {
    parser->setParsedReqBody(streamedData);
}