cmake_minimum_required(VERSION 3.10)
project(directory_ex LANGUAGES CXX)
add_library(directory_ex STATIC
    "directory_ex.cpp"
)

target_include_directories(directory_ex PUBLIC .)
auto_target_link_libraries(directory_ex PRIVATE misc libsec)
