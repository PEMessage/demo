cmake_minimum_required(VERSION 3.10)
project(project LANGUAGES CXX)
include("../../include.cmake")


# See: system/core/rootdir/ueventd.rc
message(STATUS "NOTE: ------ `chmod 0666 /dev/ashmem` before run -----")
add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE ashmem)
