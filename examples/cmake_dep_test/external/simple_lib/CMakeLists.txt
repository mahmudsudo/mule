cmake_minimum_required(VERSION 3.10)
project(simple_lib)

add_library(simple_lib STATIC src/simple_lib.cpp)
target_include_directories(simple_lib PUBLIC include)
