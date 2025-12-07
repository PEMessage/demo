CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
project(app)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-O0 -ggdb -g3)

add_executable(app main.cpp)
