cmake_minimum_required(VERSION 3.10)
project(samgr_server LANGUAGES CXX C)
include("${ROOT_DIR}/include.cmake")

add_executable(app main.cpp)
target_include_directories(app PRIVATE .)
auto_target_link_libraries(app PRIVATE samgr_mini ipc refbase hilog misc)
