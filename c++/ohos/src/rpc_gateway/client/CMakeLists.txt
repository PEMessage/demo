cmake_minimum_required(VERSION 3.10)
project(rpc_gateway_client)
include("${ROOT_DIR}/include.cmake")

# Common source files
set(COMMON_SOURCES
    ../rpc_common/event_callback_proxy.cpp
    ../rpc_common/event_callback_stub.cpp
)

# Client executable
add_executable(app
    main.cpp
    ../rpc_common/gateway_service_proxy.cpp
    ${COMMON_SOURCES}
)

target_include_directories(app PRIVATE . ../rpc_common)
auto_target_link_libraries(app PRIVATE samgr_mini ipc refbase hilog misc)
