cmake_minimum_required(VERSION 3.10)
project(syspara LANGUAGES CXX)
add_library(syspara STATIC
    src/param_comm.c
    src/param_comm.h
    src/param_wrapper.cpp
    src/parameter.c
    src/sysversion.c
)

target_include_directories(syspara PUBLIC . src include)
auto_target_link_libraries(syspara PUBLIC misc libsec init_utils)
