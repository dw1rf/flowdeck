cmake_minimum_required(VERSION 3.20)

project(FlowDeck
    VERSION 0.2.0
    DESCRIPTION "Universal command palette with window tiling and Python plugins"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 Widgets Svg Network)
include(FetchContent)
FetchContent_Declare(lua URL https://www.lua.org/ftp/lua-5.5.1.tar.gz
    URL_HASH SHA256=1c4b4068d67061f2a2231ad2b5422e77acea1487ea9890f6320af614f4373dce)
FetchContent_Populate(lua)
file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/src/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX "/(lua|luac)\\.c$")
add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC "${lua_SOURCE_DIR}/src")

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
    src/core/lua_plugin_loader.cpp
    src/core/workspace_engine.cpp
    src/core/workspace_store.cpp
    src/ui/flowdeck_controller.cpp
    resources.qrc)

target_include_directories(flowdeck PRIVATE src)

target_link_libraries(flowdeck PRIVATE Python3::Python lua_static Qt6::Core Qt6::Gui Qt6::Quick
    Qt6::Qml Qt6::QuickControls2 Qt6::Widgets Qt6::Svg Qt6::Network)

if(WIN32)
    target_link_libraries(flowdeck PRIVATE user32 shell32 dwmapi gdi32)
    add_subdirectory(plugins/example-cpp)
endif()

install(TARGETS flowdeck RUNTIME DESTINATION bin)
