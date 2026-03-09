cmake_minimum_required(VERSION 3.25)
project(thpoolcc VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

file(GLOB SOURCES "src/*.cc" "src/*.cpp")
file(GLOB INCLUDE_DIR "include")

add_executable(main ${SOURCES})

target_include_directories(main PRIVATE ${INCLUDE_DIR})