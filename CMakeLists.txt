cmake_minimum_required(VERSION 3.17)

project(LoProxy VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(loproxy 
    src/main.cpp
    src/blocking-server/BlockingServer.cpp
    src/non-blocking-server/Server.cpp
    src/engine/EpollEngine.cpp
    src/models/ConnectionState.cpp
    src/http/HttpParser.cpp
    src/http/ParseResult.cpp
    src/state/ParseReqLineState.cpp
    src/state/ParseHeaderState.cpp
    src/state/ParseCompleteState.cpp
    src/state/ParseBodyState.cpp
)   

target_include_directories(loproxy PUBLIC ${CMAKE_SOURCE_DIR}/include)