cmake_minimum_required(VERSION 3.10)
project(unicode_ex LANGUAGES CXX)
add_library(unicode_ex STATIC
    "unicode_ex.cpp"
)

target_include_directories(unicode_ex PUBLIC .)
auto_target_link_libraries(unicode_ex PRIVATE misc)
