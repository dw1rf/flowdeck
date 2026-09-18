cmake_minimum_required(VERSION 3.20)

project(FlowDeck
    VERSION 0.1.0
    DESCRIPTION "Universal command palette with window tiling and plugin system"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(flowdeck
    src/main.cpp
    src/core/hotkey_manager.cpp
    src/core/palette.cpp
    src/core/window_tiler.cpp
    src/core/plugin_manager.cpp)

target_include_directories(flowdeck PRIVATE src)

if(WIN32)
    target_link_libraries(flowdeck PRIVATE user32 shell32)
endif()

install(TARGETS flowdeck RUNTIME DESTINATION bin)
