cmake_minimum_required(VERSION 3.17)

project(LoProxy VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# =========================
# JSON library
# =========================

include(FetchContent)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_executable(loproxy 
    src/main.cpp
    src/blocking-server/BlockingServer.cpp
    src/non-blocking-server/Server.cpp
    src/engine/EpollEngine.cpp
    src/models/proxy/ConnectionState.cpp
    src/http/HttpParser.cpp
    src/http/ParseResult.cpp
    src/http/HttpResponse.cpp
    src/state/ParseReqLineState.cpp
    src/state/ParseHeaderState.cpp
    src/state/ParseCompleteState.cpp
    src/state/ParseBodyState.cpp
    src/config/Config.cpp
)

target_include_directories(loproxy PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(loproxy PRIVATE nlohmann_json::nlohmann_json)


# =========================
# Tests
# =========================

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)

FetchContent_MakeAvailable(Catch2)

add_executable(test_http_parser
    tests/unit/test_http_parser.cpp

    src/http/HttpParser.cpp
    src/http/ParseResult.cpp

    src/state/ParseReqLineState.cpp
    src/state/ParseHeaderState.cpp
    src/state/ParseCompleteState.cpp
    src/state/ParseBodyState.cpp
)

target_include_directories(test_http_parser
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_http_parser
    PRIVATE
    Catch2::Catch2WithMain
)

add_executable(test_config_json
    tests/unit/test_config_json.cpp

    src/config/Config.cpp
)

target_include_directories(test_config_json
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_config_json
    PRIVATE
    Catch2::Catch2WithMain
)

target_link_libraries(test_config_json
    PRIVATE
    nlohmann_json::nlohmann_json
)

