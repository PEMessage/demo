cmake_minimum_required(VERSION 3.14)
project(libsec)

include(FetchContent)

# Fetch content using git clone
FetchContent_Declare(
    libsec_sources
    GIT_REPOSITORY https://gitee.com/openharmony/third_party_bounds_checking_function.git
    GIT_TAG        OpenHarmony-v5.1.0-Release
    SOURCE_DIR     ${CMAKE_CURRENT_LIST_DIR}/code
)

FetchContent_MakeAvailable(libsec_sources)

# Get all source files from src directory
file(GLOB_RECURSE LIB_SEC_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/code/src/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/code/src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/code/src/*.cc"
)


add_library(libsec STATIC
    "${LIB_SEC_SOURCES}"
)

target_include_directories(libsec PUBLIC code/include)
target_include_directories(libsec PRIVATE code/src)
