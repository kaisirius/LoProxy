#include <state/ParseHeaderState.hpp>
#include <state/ParseBodyState.hpp>
#include <state/ParseCompleteState.hpp>
#include <http/HttpParser.hpp>
#include <regex>

ParseResult ParseHeaderState::handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser) {
    ParseResult res(Status(INCOMPLETE), 0);

    for(size_t i = startIdx; i < (size_t)data.length(); ++i) {
        streamedData += data[i];
        res.bytes_consumed++;
        // checking \r 
        // two cases we signal end of headers either prev was \n or if no header no charac before \r
        int lenStreamed = streamedData.length();
        if((streamedData[lenStreamed - 1] == '\r' && lenStreamed == 1) || (streamedData[lenStreamed - 1] == '\r' && streamedData[lenStreamed - 2] == '\n')) {
            streamedData += '\n';

            if(isExtractable()) {
                extractFromStreamedData(parser);
        
                if((parser->getParsedReqObj().method == "POST" || parser->getParsedReqObj().method == "PUT") && parser->getParsedReqObj().headers.find("Content-Length") != parser->getParsedReqObj().headers.end()) {
                
                    ssize_t contentLength = std::stoi(parser->getParsedReqObj().headers.at("Content-Length"));
                    parser->currentState = std::make_unique<ParseBodyState>();
                    parser->currentState->contentLength_ToParse = contentLength;

                } else {
                    parser->currentState = std::make_unique<ParseCompleteState>();
                }
                ParseResult nextStateRes = parser->currentState.get()->handleHTTPparsing(data, i + 2, parser); // skipping \n charac
                
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

bool ParseHeaderState::isExtractable() {
    std::regex headersRegex(R"((([!#$%&'*+\-.^_`|~0-9A-Za-z]+:[ \t]*[^\r\n]*\r\n)*)\r\n)");
    return std::regex_match(streamedData, headersRegex);
}

void ParseHeaderState::extractFromStreamedData(HttpParser* parser) {
    bool colonIterated = false;
    std::string fieldKey = "", fieldValue = "";
    for(size_t i = 0; i < (size_t)streamedData.length(); ++i) {
        if(streamedData[i] == '\r') {
            ++i; // loop's ++i will skip \n and goes to next header field starting point
            colonIterated = false;
            if(fieldKey != "") {
                parser->setParsedReqHeader(fieldKey, fieldValue);
            } 
            fieldKey = "";
            fieldValue = "";
        } else if(streamedData[i] == ':') {
          colonIterated = true;
        } else if(streamedData[i] == ' ' && (int)fieldValue.length() == 0) { // only first ' ' to be skipped
            continue;
        } else {
            if(!colonIterated) fieldKey += streamedData[i];
            else fieldValue += streamedData[i];
        }
    }
}