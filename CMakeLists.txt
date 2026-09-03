cmake_minimum_required(VERSION 4.3)
project(hello_boost)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost CONFIG REQUIRED COMPONENTS filesystem log)

add_executable(hello_boost main.cpp)
target_link_libraries(hello_boost PRIVATE Boost::filesystem Boost::log Boost::headers)
add_executable(client client.cpp)
target_link_libraries(client PRIVATE Boost::log Boost::headers)
