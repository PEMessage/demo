add_executable(echo_server main.c)
target_link_libraries(echo_server PRIVATE uv_a)
