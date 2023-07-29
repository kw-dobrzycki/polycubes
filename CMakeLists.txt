cmake_minimum_required(VERSION 3.25)
project(tetris)

set(CMAKE_CXX_STANDARD 23)

add_executable(tetris main.cpp include/poly.h include/relative.h src/relative.cpp tests/test.cpp include/groups.h src/groups.cpp)
