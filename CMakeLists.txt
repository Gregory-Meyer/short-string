cmake_minimum_required(VERSION 3.9)
project(small_string)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -Wconversion -pedantic -pedantic-errors -Wshadow -Wcast-qual -march=native")
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS} -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -flto -fgcse-sm -fgcse-las -fgcse-after-reload")

include_directories(include/)

add_executable(small_string src/main.cpp include/small_string.hpp)

add_executable(test_basic test/test_basic.cpp test/catch.hpp test/catch_base.cpp
               include/small_string.hpp)
