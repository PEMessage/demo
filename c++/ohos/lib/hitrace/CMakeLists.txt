cmake_minimum_required(VERSION 3.10)
project(hitrace LANGUAGES CXX C)
add_library(hitrace STATIC
    frameworks/native/c_wrapper/source/hitracechain_c_wrapper.c
    frameworks/native/hitracechainc.c

    frameworks/native/hitraceid.cpp
    frameworks/native/hitracechain.cpp
    frameworks/native/dynamic_buffer.cpp

    utils/trace_json_parser.cpp
    utils/common_utils.cpp
    utils/trace_file_utils.cpp
    utils/file_ageing_utils.cpp

    interfaces/native/innerkits/src/hitrace_meter_wrapper.cpp
    interfaces/native/innerkits/src/hitrace_meter_c.c
    interfaces/native/innerkits/src/hitrace_meter_mock.cpp
)

target_include_directories(hitrace PUBLIC
    interfaces/native/innerkits/include
    interfaces/native/innerkits/include/hitrace_meter

    frameworks/include
    frameworks/native/c_wrapper/include

    common
    utils
)

target_include_directories(hitrace PRIVATE
    frameworks/native
)

auto_target_link_libraries(hitrace PUBLIC libsec hilog nocopyable cjson syspara)
