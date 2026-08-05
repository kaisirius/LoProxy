#pragma once
#include <string>
#include <unordered_map>

struct ParsedRequest {
    std::string method = "";
    std::string uri = "";
    std::string version = "HTTP/1.1";
    std::unordered_map<std::string, std::string> headers;
    std::string body = "";
    bool complete = false;
};