add_executable(uv_coro main.cpp)
target_link_libraries(uv_coro PRIVATE uv_a)
target_compile_features(uv_coro PRIVATE cxx_std_20)
