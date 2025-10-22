CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
project(ioctl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(visit_struct)
add_compile_options(-O0 -ggdb -g3)

add_executable(ioctl ioctl.cpp)
target_link_libraries(ioctl PUBLIC visit_struct)

# add_executable(read_mod_write read_mod_write.cpp)
# target_link_libraries(read_mod_write PUBLIC visit_struct)
