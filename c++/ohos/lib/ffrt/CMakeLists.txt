cmake_minimum_required(VERSION 3.10)
project(ffrt LANGUAGES CXX C)
add_library(ffrt STATIC
    keep.c
)

target_include_directories(ffrt PUBLIC
    interfaces/kits
    interfaces/inner_api
)

auto_target_link_libraries(ffrt PUBLIC libsec nocopyable misc hilog)
