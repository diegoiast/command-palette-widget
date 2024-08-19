#
# This is an eample/demo of the command pallete library.
#

cmake_minimum_required(VERSION 3.16)

project(CommandPaletteWidgetExample VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_subdirectory(../ build)

# alternative option - use CPM - https://github.com/cpm-cmake/CPM.cmake
# include(CPM.cmake) 
# CPMAddPackage("gh:diegoiast/command-palette-widget#main") 


# alternative option: copy the following files to your project
# include/CommandPaletteWidget/CommandPalette
# include/CommandPaletteWidget/commandpalette.h
# src/commandpalette.cpp

add_executable(CommandPaletteWidgetExample
        main.cpp
)
target_link_libraries(CommandPaletteWidgetExample PRIVATE CommandPaletteWidget Qt::Widgets)
