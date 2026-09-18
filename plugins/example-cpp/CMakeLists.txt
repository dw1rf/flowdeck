cmake_minimum_required(VERSION 3.20)

project(flowdeck_example_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
endif()

add_library(flowdeck_example_cpp SHARED
    plugin.cpp)

target_include_directories(flowdeck_example_cpp PRIVATE ../../src)

set_target_properties(flowdeck_example_cpp PROPERTIES
    PREFIX ""
    OUTPUT_NAME "example_cpp")
