# CMSDK_CM3 Library
cmake_minimum_required(VERSION 3.12)
project(CMSDK_CM3_EXT LANGUAGES C ASM)
set(MODULE_NAME CMSDK_CM3_EXT)

add_library(${MODULE_NAME} STATIC
    keep.c
)

target_link_libraries(${MODULE_NAME}
    PRIVATE
    CMSDK_CM3
)
target_include_directories(${MODULE_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

