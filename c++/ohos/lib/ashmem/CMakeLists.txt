cmake_minimum_required(VERSION 3.10)
project(ashmem LANGUAGES CXX)
add_library(ashmem STATIC
    "ashmem.cpp"
)

target_include_directories(ashmem PUBLIC .)
auto_target_link_libraries(ashmem PUBLIC misc refbase parcel)
