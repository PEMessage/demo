cmake_minimum_required(VERSION 3.10)
project(demo_service_server LANGUAGES CXX C)
include("../../include.cmake")

add_executable(demo_service_server main.cpp
    ../rpc_common/demo_service_stub.cpp
)
target_include_directories(demo_service_server PRIVATE 
    .
    ../rpc_common
)
auto_target_link_libraries(demo_service_server PRIVATE 
    samgr_mini 
    ipc 
    refbase 
    hilog 
    misc
)
