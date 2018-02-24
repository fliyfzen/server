cmake_minimum_required(VERSION 2.8)

project(server)

set(SOURCES
    "src/main.cpp"
    "src/generator.cpp"
    "src/network.cpp"
)

INCLUDE_DIRECTORIES(src/include)

add_executable(${PROJECT_NAME} ${SOURCES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread -Wall -pedantic")
