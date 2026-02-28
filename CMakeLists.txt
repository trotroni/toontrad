cmake_minimum_required(VERSION 3.21)
project(TestQt)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(TestQt main.cpp)

target_link_libraries(TestQt Qt6::Widgets)