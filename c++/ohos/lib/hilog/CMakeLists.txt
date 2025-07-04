cmake_minimum_required(VERSION 3.10)
project(hilog LANGUAGES CXX)
add_library(hilog STATIC
    frameworks/libhilog/base/hilog_base.c
)

# fix CMake Error: Cannot determine link language for target "hilog".
set_target_properties(hilog PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(hilog PUBLIC
    frameworks/include
    frameworks/libhilog/include
    interfaces/native/innerkits/inclue
)
# auto_target_link_libraries(hilog PUBLIC misc refbase libsec)
