cmake_minimum_required(VERSION 3.10)
project(init_utils LANGUAGES CXX C)
add_library(init_utils STATIC
    src/init_hashmap.c
    src/init_utils.c
    src/init_log.c
    src/init_commlog.c
)

target_include_directories(init_utils PUBLIC . include)
auto_target_link_libraries(init_utils PUBLIC misc libsec)
