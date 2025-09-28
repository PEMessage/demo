cmake_minimum_required(VERSION 3.10)
project(project LANGUAGES CXX C)
include("../../include.cmake")


add_executable(app
    main.cpp
    node_handle.cpp
    node_device.cpp
)
auto_target_link_libraries(app PRIVATE timer nocopyable)
