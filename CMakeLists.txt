cmake_minimum_required(VERSION 3.25)
project(tetris)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS -static-libstdc++)

find_package(OpenMP REQUIRED)

add_executable(tetris
        main.cpp
        include/poly.h
        include/relative.h
        src/relative.cpp
        tests/test.cpp
        include/groups.h
        src/groups.cpp
        include/Tet.h
        src/Tet.cpp
        )

target_include_directories(tetris PUBLIC include)

target_link_libraries(tetris PRIVATE OpenMP::OpenMP_CXX)