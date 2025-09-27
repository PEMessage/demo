cmake_minimum_required(VERSION 3.10)
project(thread_ex LANGUAGES CXX)
add_library(thread_ex STATIC
    "thread_ex.cpp"
)

target_include_directories(thread_ex PUBLIC .)
auto_target_link_libraries(thread_ex PRIVATE misc)
