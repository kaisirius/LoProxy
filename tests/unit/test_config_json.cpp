#include <catch2/catch_test_macros.hpp>
#include <config/Config.hpp>
#include <nlohmann/json.hpp>

TEST_CASE("Valid JSON config parsing") {
    Config config = Config::load("./config/config.json");
    REQUIRE(config.listeningHost == "127.0.0.1");
    REQUIRE(config.listeningPort == 8080);
    REQUIRE(config.upstreams.size() == 3);
    REQUIRE(config.upstreams[0].host == "127.0.0.1");
    REQUIRE(config.upstreams[0].port == 3001);
}


TEST_CASE("Missing field JSON config parsing") {
    REQUIRE_THROWS_AS(
        Config::load("./config/missing_field_config.json"),
        nlohmann::json::out_of_range
    );
}


TEST_CASE("Type mismatch in JSON config parsing") {
    REQUIRE_THROWS_AS(
        Config::load("./config/type_mismatch_config.json"),
        nlohmann::json::type_error
    );
}
