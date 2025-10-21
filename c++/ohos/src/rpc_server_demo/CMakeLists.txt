cmake_minimum_required(VERSION 3.10)
project(project LANGUAGES CXX C)
include("../../include.cmake")


add_executable(app main.cpp rpc_test.cpp)
target_include_directories(app PRIVATE .)
auto_target_link_libraries(app PRIVATE ipc ashmem)
