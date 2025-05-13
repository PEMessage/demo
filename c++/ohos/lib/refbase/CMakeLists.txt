cmake_minimum_required(VERSION 3.10)
project(refbase LANGUAGES CXX)
add_library(refbase STATIC
    "refbase.cpp"
)

target_include_directories(refbase PUBLIC .)

target_compile_definitions(refbase PUBLIC 
    DEBUG_REFBASE
    PRINT_TRACK_AT_ONCE
    TRACK_ALL
)
