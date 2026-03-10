cmake_minimum_required(VERSION 3.25)
project(thpoolcc VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)

file(GLOB SOURCES "*.cc" "*.cpp")

add_executable(perf ${SOURCES})

target_include_directories(perf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})