cmake_minimum_required(VERSION 4.0)

# Get current folder name
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(${PROJECT_NAME})

set(CMAKE_C_STANDARD 11)

add_executable(${PROJECT_NAME} main.c)