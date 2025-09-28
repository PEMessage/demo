cmake_minimum_required(VERSION 3.10)
project(file_ex LANGUAGES CXX)
add_library(file_ex STATIC
    "file_ex.cpp"
)

target_include_directories(file_ex PUBLIC .)
auto_target_link_libraries(file_ex PRIVATE misc directory_ex)
