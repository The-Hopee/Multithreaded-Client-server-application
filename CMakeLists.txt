cmake_minimum_required(VERSION 3.5)

project(server)

include_directories(include)

find_library(LIBCRYPTOPP_PATH cryptopp)

message(STATUS "Path to libcryptopp: ${LIBCRYPTOPP_PATH}")

add_library(rtt::libcryptopp STATIC IMPORTED)

set_target_properties(rtt::libcryptopp 
 PROPERTIES
 IMPORTED_LOCATION ${LIBCRYPTOPP_PATH} 
 INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include/cryptopp"# где найти библиотеки
)

add_executable(server src/server.cpp)
add_executable(client src/client.cpp)

target_link_libraries(server PRIVATE rtt::libcryptopp)
target_link_libraries(client PRIVATE rtt::libcryptopp)