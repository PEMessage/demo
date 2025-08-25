cmake_minimum_required(VERSION 3.10)
project(hilog LANGUAGES CXX C)
add_library(hilog STATIC
    # frameworks/libhilog/base/hilog_base.c
    frameworks/libhilog/hilog.cpp
    frameworks/libhilog/hilog_printf.cpp
    frameworks/libhilog/vsnprintf/vsprintf_p.c
    frameworks/libhilog/vsnprintf/vsnprintf_s_p.c
    # frameworks/libhilog/param/properties.cpp
    frameworks/libhilog/utils/log_print.cpp
    frameworks/libhilog/utils/log_utils.cpp
)

# fix CMake Error: Cannot determine link language for target "hilog".
# set_target_properties(hilog PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(hilog PUBLIC
    frameworks/include
    frameworks/libhilog/include
    interfaces/native/innerkits/include
    frameworks/libhilog/utils/include
    frameworks/libhilog/vsnprintf/include
    # frameworks/libhilog/param/include
)

# It will include "log_print.h", in hilog_printf.cpp for LogPrintWithFormat
# log_print.h default out is std::cout
# PrintLog will used it for none-ohos platform,
target_compile_definitions(hilog PRIVATE __LINUX__)
# auto_target_link_libraries(hilog PUBLIC libsec syspara)
auto_target_link_libraries(hilog PUBLIC libsec)
