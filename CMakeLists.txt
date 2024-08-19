#
# This is the library of the command pallete library.
#
# To test this - use the example/CMakeLists.txt
# This CMakeList.txt should be included, not built directly

cmake_minimum_required(VERSION 3.16)

project(CommandPaletteWidget VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(CommandPaletteWidget
    src/commandpalette.cpp
    include/CommandPaletteWidget/commandpalette.h 
)
target_link_libraries(CommandPaletteWidget PUBLIC Qt::Widgets)
target_include_directories(CommandPaletteWidget PUBLIC include)
target_include_directories(CommandPaletteWidget PRIVATE include)
