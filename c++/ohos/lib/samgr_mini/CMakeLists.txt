cmake_minimum_required(VERSION 3.10)
project(samgr_mini LANGUAGES CXX C)

# 查找项目根目录
include("${CMAKE_CURRENT_SOURCE_DIR}/../../include.cmake")

# 创建库
add_library(samgr_mini STATIC
    samgr_mini.cpp
)

# 设置包含目录
target_include_directories(samgr_mini PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接依赖库
auto_target_link_libraries(samgr_mini PUBLIC ipc refbase hilog misc)

# 导出头文件
set_target_properties(samgr_mini PROPERTIES
    PUBLIC_HEADER "samgr_mini.h;iservice_registry.h;isystem_ability_manager.h"
)
