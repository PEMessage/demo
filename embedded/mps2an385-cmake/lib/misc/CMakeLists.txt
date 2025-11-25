cmake_minimum_required(VERSION 3.12)
project(misc LANGUAGES C ASM)

add_library(misc INTERFACE)

target_include_directories(misc
                           INTERFACE
                           include/)
