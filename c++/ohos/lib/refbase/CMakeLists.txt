cmake_minimum_required(VERSION 3.10)
project(refbase LANGUAGES CXX)

add_library(refbase STATIC
    "refbase.cpp"
)
target_include_directories(refbase PUBLIC .)
auto_target_link_libraries(refbase PUBLIC misc)




add_library(refbase+debug_all STATIC
    "refbase.cpp"
)
target_include_directories(refbase+debug_all PUBLIC .)
auto_target_link_libraries(refbase+debug_all PUBLIC misc)
target_compile_definitions(refbase+debug_all PUBLIC
    DEBUG_REFBASE
    PRINT_TRACK_AT_ONCE
    TRACK_ALL
)

add_library(refbase+debug STATIC
    "refbase.cpp"
)
target_include_directories(refbase+debug PUBLIC .)
auto_target_link_libraries(refbase+debug PUBLIC misc)
target_compile_definitions(refbase+debug PUBLIC
    DEBUG_REFBASE
    PRINT_TRACK_AT_ONCE
    TRACK_ALL
)
