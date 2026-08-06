#include <state/ParseReqLineState.hpp>
#include <http/HttpParser.hpp>
#include <state/ParseHeaderState.hpp>
#include <regex>
#include <iostream>

ParseResult ParseReqLineState::handleHTTPparsing(const std::string& data, int startIdx, HttpParser* parser) {
    ParseResult res(Status(COMPLETE), 0);

    for(size_t i = startIdx; i < (size_t)data.length(); ++i) {
        streamedData += data[i];
        res.bytes_consumed++;
        // ig no need to check for last to last character as \r
        if(streamedData[streamedData.length() - 1] == '\n') {
            if(isExtractable()) {
                try {
                    extractFromStreamedData(parser);

                    parser->currentState = std::make_unique<ParseHeaderState>();
                    ParseResult nextStateRes = parser->currentState.get()->handleHTTPparsing(data, i + 1, parser); 
                    res.status = nextStateRes.status;
                    res.bytes_consumed += nextStateRes.bytes_consumed;
                } catch(const char* msg) {
                    if(msg == "INVALID METHOD") {
                        res.status = ERROR;
                        parser->reset();
                    }
                }               
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
    std::regex reqLineRegex("([A-Z]+)\\s(/\\S*)\\sHTTP/1\\.1\r\n");
    return std::regex_match(streamedData, reqLineRegex);
}

void ParseReqLineState::extractFromStreamedData(HttpParser* parser) {    
    std::string _method = "";
    std::string _uri = "";
    bool spaceIterated = false;

    for(size_t i = 0; i < (size_t)streamedData.length(); ++i) {
        if(streamedData[i] == ' ') {
            if(spaceIterated) break;
            spaceIterated = true;
            continue;
        }
        if(spaceIterated) {
            _uri += streamedData[i];  
        } else {
            _method += streamedData[i];
        }
    }

    if(_method == "GET" | _method == "PUT" | _method == "POST" | _method == "DELETE") {
        parser->setParsedReqMethod(_method);
        parser->setParsedReqURI(_uri);
    } else {
        throw "INVALID METHOD";
    }
 }