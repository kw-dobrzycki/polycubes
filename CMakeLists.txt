cmake_minimum_required(VERSION 3.25)
project(tetris)

set(CMAKE_CXX_STANDARD 23)

add_executable(tetris
        main.cpp
        poly.h
        relative.h
        relative.cpp
        test.cpp
        test.h
        groupGenerators.h
        groupGenerators.cpp)
