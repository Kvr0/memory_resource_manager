cmake_minimum_required(VERSION 3.10)

project(mrm_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_heap test_heap.cpp)
