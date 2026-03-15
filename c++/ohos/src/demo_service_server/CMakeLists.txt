cmake_minimum_required(VERSION 3.10)
project(app LANGUAGES CXX C)
include("../../include.cmake")

add_executable(app main.cpp
    ../rpc_common/demo_service_stub.cpp
)
target_include_directories(app PRIVATE
    .
    ../rpc_common
)
auto_target_link_libraries(app PRIVATE
    samgr_mini 
    ipc 
    refbase 
    hilog 
    misc
)
