cmake_minimum_required(VERSION 2.8.12.2)
project(Pager)
set(CMAKE_C_STANDARD 99)
add_compile_options(-Wall -Wextra -pedantic -Werror -g)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O0")
set(CMAKE_BUILD_TYPE Debug)#also can use: -DCMAKE_BUILD_TYPE=Debug
include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(pager ${CMAKE_SOURCE_DIR}/pager.c)