cmake_minimum_required(VERSION 3.5)

project(application)

include_directories(include)

add_executable(server src/server.cpp)
add_executable(client src/client.cpp)