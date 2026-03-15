cmake_minimum_required(VERSION 3.10)
project(demo_client LANGUAGES CXX C)
include("../../include.cmake")

add_executable(demo_client main.cpp
    ../rpc_common/demo_service_stub.cpp
    ../rpc_common/demo_service_proxy.cpp
)
target_include_directories(demo_client PRIVATE 
    .
    ../rpc_common
)
auto_target_link_libraries(demo_client PRIVATE 
    samgr_mini 
    ipc 
    refbase 
    hilog 
    misc
)
