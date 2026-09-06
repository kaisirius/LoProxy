#include <non-blocking-server/Server.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <iostream>
using json = nlohmann::json;

int main() {
    try {
        std::string pathToFile = "./config/config.json";
        Config config = Config::load(pathToFile);
        std::unique_ptr<Server> server = std::make_unique<Server>(config);
        server.get()->init();
        server.get()->runEventLoop();
        
    } catch(json::parse_error err) {
        std::cerr << "[ERROR]: Parsing error due to invalid JSON format." << "\n";  
        throw std::runtime_error(err.what());
    } catch(json::out_of_range err) {
        std::cerr << "[ERROR]: Parsing error due to missing key." << "\n";  
        throw std::runtime_error(err.what());
    } catch(json::type_error err) {
        std::cerr << "[ERROR]: Parsing error due to type mismatch in config file." << "\n";  
        throw std::runtime_error(err.what());
    }
    
    
    return 0;
}