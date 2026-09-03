cmake_minimum_required(VERSION 4.3)
project(hello_boost)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost CONFIG REQUIRED COMPONENTS log)

add_executable(server server.cpp)
target_link_libraries(server PRIVATE Boost::log Boost::headers)
add_executable(client client.cpp)
target_link_libraries(client PRIVATE Boost::log Boost::headers)
