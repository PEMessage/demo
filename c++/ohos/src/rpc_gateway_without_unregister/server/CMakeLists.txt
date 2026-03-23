cmake_minimum_required(VERSION 3.10)
project(rpc_gateway_server)
include("${ROOT_DIR}/include.cmake")

# Common source files
set(COMMON_SOURCES
    ../rpc_common/event_callback_proxy.cpp
    ../rpc_common/event_callback_stub.cpp
)

# Server executable
add_executable(app
    main.cpp
    backend_service_impl.cpp
    ../rpc_common/backend_service_stub.cpp
    ${COMMON_SOURCES}
)

target_include_directories(app PRIVATE . ../rpc_common)
auto_target_link_libraries(app PRIVATE samgr_mini ipc refbase hilog misc)
