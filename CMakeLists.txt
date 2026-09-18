cmake_minimum_required(VERSION 3.20)

project(FlowDeck
    VERSION 0.2.0
    DESCRIPTION "Universal command palette with window tiling and Python plugins"
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

# --- Embedded CPython --------------------------------------------------------
# Needs Python 3.11+ with development headers.
# Windows: install from python.org, then pass -DPython3_ROOT_DIR="C:/Python312".
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter Development.Embed)

message(STATUS "FlowDeck: using Python ${Python3_VERSION} at ${Python3_EXECUTABLE}")

add_executable(flowdeck WIN32
    src/main.cpp
    src/core/hotkey_manager.cpp
    src/core/palette.cpp
    src/core/fuzzy.cpp
    src/core/window_tiler.cpp
    src/core/plugin_manager.cpp
    src/core/python_runtime.cpp
    src/core/python_plugin_loader.cpp
    src/ui/launcher_window.cpp)

target_include_directories(flowdeck PRIVATE src)

target_link_libraries(flowdeck PRIVATE Python3::Python)

if(WIN32)
    target_link_libraries(flowdeck PRIVATE user32 shell32 dwmapi gdi32)
    add_subdirectory(plugins/example-cpp)
endif()

install(TARGETS flowdeck RUNTIME DESTINATION bin)
