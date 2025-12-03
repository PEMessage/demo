cmake_minimum_required(VERSION 3.10)
project(project LANGUAGES CXX)
include("../../include.cmake")


add_executable(app main.cpp persistable_bundle.cpp)
target_include_directories(app PUBLIC .)
auto_target_link_libraries(app PRIVATE parcel misc string_ex)
