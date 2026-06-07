cmake_minimum_required(VERSION 3.17)

project(LoProxy VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(loproxy 
    src/main.cpp
    src/blocking-server/Server.cpp
)   

target_include_directories(loproxy PUBLIC ${CMAKE_SOURCE_DIR}/include)