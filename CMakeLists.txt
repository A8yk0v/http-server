cmake_minimum_required(VERSION 2.8)

project(final)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

set (SOURCES main.c)

add_executable(${PROJECT_NAME} "main.c")
target_link_libraries(${PROJECT_NAME} pthread)
