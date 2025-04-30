
set(LV_CONF_INCLUDE_SIMPLE OFF CACHE BOOL "Use simple include for lv_conf.h" FORCE)
add_subdirectory(lvgl)
add_subdirectory(lvgl_port)

cmake_policy(SET CMP0079 NEW)
target_link_libraries(lvgl PRIVATE freertos_kernel)

