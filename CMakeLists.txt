cmake_minimum_required(VERSION 3.16)
project(lalloc C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "-O3")

add_executable(lalloc main.c lalloc.h util.h)