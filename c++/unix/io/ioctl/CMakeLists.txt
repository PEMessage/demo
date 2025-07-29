CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
project(ioctl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(visit_struct)

add_executable(ioctl ioctl.cpp)
target_link_libraries(ioctl PUBLIC visit_struct)

