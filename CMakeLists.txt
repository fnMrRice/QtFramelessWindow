cmake_minimum_required(VERSION 3.14)

# Toast组件, 需要至少一个窗口用于显示
project(QtFramelessWindow VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 5.15 COMPONENTS Core Gui Widgets REQUIRED)

set(PROJECT_SOURCES
        include/QtFramelessWindow.h
        src/QtFramelessWindow.cpp)

add_library(QtFramelessWindow ${PROJECT_SOURCES})
target_include_directories(QtFramelessWindow PRIVATE include)

if (MSVC)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif ()

target_link_libraries(QtFramelessWindow PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        dwmapi user32)
