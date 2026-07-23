cmake_minimum_required(VERSION 3.14)
project(multi-project LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  libuv
  GIT_REPOSITORY https://github.com/libuv/libuv.git
  GIT_TAG v1.49.2
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libuv)

add_subdirectory(src/uv_version)
