cmake_minimum_required(VERSION 3.10)
project(samgr_server LANGUAGES CXX C)
include("../../include.cmake")

add_executable(samgr_server main.cpp)
target_include_directories(samgr_server PRIVATE .)
auto_target_link_libraries(samgr_server PRIVATE samgr_mini ipc refbase hilog misc)
