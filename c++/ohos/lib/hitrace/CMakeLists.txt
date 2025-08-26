cmake_minimum_required(VERSION 3.10)
project(hitrace LANGUAGES CXX C)
add_library(hitrace STATIC
    keep.c
)

target_include_directories(hitrace PUBLIC
    interfaces/native/innerkits/include
)

auto_target_link_libraries(hitrace PUBLIC libsec)
