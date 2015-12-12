cmake_minimum_required(VERSION 2.8)
project(cucumbersome C)

add_definitions(-std=c99 -DDEBUG -Wall -g)

set(CMAKE_BUILD_TYPE Debug)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cog/src/CMake" ${CMAKE_MODULE_PATH})
include_directories("${CMAKE_SOURCE_DIR}/cog/src" "${CMAKE_SOURCE_DIR}")

add_subdirectory(cog/src)

file(GLOB srcs *.c)
add_executable(cucumbersome ${srcs})
target_link_libraries(cucumbersome cog)
