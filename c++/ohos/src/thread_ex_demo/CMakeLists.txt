cmake_minimum_required(VERSION 3.10)
project(project LANGUAGES CXX C)
include("../../include.cmake")


add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE thread_ex)
