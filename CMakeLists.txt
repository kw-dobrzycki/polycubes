cmake_minimum_required(VERSION 3.25)
project(tetris)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS -static-libstdc++)

add_executable(tetris
        main.cpp
        include/poly.h
        include/Tet.h
)

target_include_directories(tetris PUBLIC include)