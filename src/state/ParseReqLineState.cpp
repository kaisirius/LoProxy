#include <state/ParseReqLineState.hpp>
#include <state/ParseHeaderState.hpp>

ParseResult ParseReqLineState::handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser) {
    ParseResult res(Status(COMPLETE), 0);

    for(size_t i = startIdx; i < (size_t)data.length(); ++i) {
        streamedData += data[i];
        res.bytes_consumed++;
        // ig no need to check for last to last character as \r
        if(streamedData[streamedData.length() - 1] == '\n') {
            if(isExtractable()) {
                extractFromStreamedData();

                parser->currentState = std::make_unique<ParseHeaderState>();
                ParseResult nextStateRes = parser->currentState.get()->handleHTTPparsing(data, i + 1, parser); 
                res.status = nextStateRes.status;
                res.bytes_consumed += nextStateRes.bytes_consumed;
            } else {
                res.status = ERROR;
                parser->reset();
            }
            break;
        } 
    }

    return res;
}

bool ParseReqLineState::isExtractable() {

}

void ParseReqLineState::extractFromStreamedData() {

}