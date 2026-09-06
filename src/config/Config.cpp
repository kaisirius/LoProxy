#include <config/Config.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <iostream>

using json = nlohmann::json;

Config Config::load(const std::string& path) {
    std::ifstream configFile(path);
    if (!configFile.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    Config cfg;
    
    json extractedJson = json::parse(configFile);
    cfg.listeningHost = extractedJson.at("listen_host").get<std::string>();
    cfg.listeningPort = extractedJson.at("listen_port").get<int32_t>();
    cfg.lbStrategy = extractedJson.at("lb_strategy").get<std::string>();
    
    const auto& arr = extractedJson.at("upstreams");
    if (!arr.is_array())
        throw std::runtime_error("'upstreams' must be an array");

    for (const auto& item : arr) {
        UpstreamServer upstream;
        upstream.host = item.at("host").get<std::string>();
        upstream.port = item.at("port").get<int>();
        cfg.upstreams.push_back(std::move(upstream));
    }
    
    return cfg;
}