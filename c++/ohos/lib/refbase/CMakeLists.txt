cmake_minimum_required(VERSION 3.10)
project(refbase LANGUAGES CXX)
add_library(refbase STATIC
    "refbase.cpp"
)

target_include_directories(refbase PUBLIC .)
auto_target_link_libraries(refbase PUBLIC misc)

target_compile_definitions(refbase PUBLIC 
    DEBUG_REFBASE
    PRINT_TRACK_AT_ONCE
    TRACK_ALL
)
