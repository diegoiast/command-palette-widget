cmake_minimum_required(VERSION 3.24)
project(FuzzyTest)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Test REQUIRED)

add_executable(FuzzyTest
    test_fuzzy.cpp
    ../src/fuzzy.cpp
)

target_link_libraries(FuzzyTest
    Qt::Core
    Qt::Test
)

add_test(NAME FuzzyTest COMMAND FuzzyTest)
