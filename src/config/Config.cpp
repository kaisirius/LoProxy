#include <config/Config.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <set>

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

    std::set<std::pair<std::string, int32_t>> uniqueConfigs;

    for (const auto& item : arr) {
        if(uniqueConfigs.find({item.at("host").get<std::string>(), item.at("port").get<int32_t>()}) == uniqueConfigs.end()) {
            UpstreamConfig upstreamConfig;
            upstreamConfig.host = item.at("host").get<std::string>();
            upstreamConfig.port = item.at("port").get<int32_t>();

            cfg.upstreams.push_back(std::move(upstreamConfig));

            uniqueConfigs.insert({item.at("host").get<std::string>(), item.at("port").get<int32_t>()});
        }
    }
    if(uniqueConfigs.size() == 0) {
        throw std::runtime_error("At least one upstream to be present");
    }
    return cfg;
}