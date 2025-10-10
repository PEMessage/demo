cmake_minimum_required(VERSION 3.10)
project(project LANGUAGES CXX C)
include("../../include.cmake")


add_executable(app
    main.cpp
    node_device.cpp
    node_handle.cpp
    node_handles.cpp
    node_manager.cpp

)

target_compile_options(app PRIVATE -fsanitize=address,leak,undefined)
target_compile_options(app PRIVATE -fstack-protector)
target_link_options(app PRIVATE -fsanitize=address,leak,undefined)
target_link_options(app PRIVATE -fstack-protector)

auto_target_link_libraries(app PRIVATE timer nocopyable)
