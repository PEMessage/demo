# CMSIS Library
cmake_minimum_required(VERSION 3.12)
project(CMSIS LANGUAGES C)


# Create library
add_library(CMSIS INTERFACE)

target_include_directories(CMSIS
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Driver/Include
)
