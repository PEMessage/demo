cmake_minimum_required(VERSION 3.10)
project(project LANGUAGES CXX C)
include("../../include.cmake")


add_executable(app
    server_main.cpp
)
target_include_directories(app PUBLIC .)

auto_target_link_libraries(app PRIVATE ipc ashmem misc)
