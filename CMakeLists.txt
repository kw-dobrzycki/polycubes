cmake_minimum_required(VERSION 3.25)
project(tetris)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "-static -fopenmp -ggdb")

add_executable(tetris
        main.cpp
        include/poly.h
        include/Tet.h
)

find_package(OpenMP REQUIRED)

target_include_directories(tetris PUBLIC include)
target_link_libraries(tetris PRIVATE OpenMP::OpenMP_CXX)