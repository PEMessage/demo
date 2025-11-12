# CMSDK_CM3 Library
cmake_minimum_required(VERSION 3.12)
project(CMSDK_CM3 LANGUAGES C ASM)

add_library(CMSDK_CM3_EXT STATIC keep.c)

target_include_directories(CMSDK_CM3_EXT
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

