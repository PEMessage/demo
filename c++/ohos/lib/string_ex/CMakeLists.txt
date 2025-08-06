cmake_minimum_required(VERSION 3.10)
project(string_ex LANGUAGES CXX)
add_library(string_ex STATIC
    "string_ex.cpp"
)

target_include_directories(string_ex PUBLIC .)
auto_target_link_libraries(string_ex PRIVATE misc unicode_ex)
