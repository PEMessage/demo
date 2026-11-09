cmake_minimum_required(VERSION 3.10)
project(refbase_self_ref LANGUAGES CXX)
include("${ROOT_DIR}/include.cmake")

add_executable(app main.cpp)
auto_target_link_libraries(app PRIVATE refbase)
