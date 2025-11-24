add_library(lvgl_port
    STATIC
    lv_port_disp.c
    lv_port_indev.c
)
target_include_directories(lvgl_port PUBLIC
    ./
)
target_link_libraries(lvgl_port PRIVATE lvgl freertos_kernel CMSDK_CM3_EXT)

add_library(lvgl_port_with_multitouch
    STATIC
    lv_port_disp.c
    lv_port_indev.c
)
target_include_directories(lvgl_port_with_multitouch PUBLIC
    ./
)

target_compile_definitions(lvgl_port_with_multitouch
    PRIVATE
    LV_PORT_INDEV_SUPPORT_MULTI_TOUCH=1
)
target_link_libraries(lvgl_port_with_multitouch PRIVATE lvgl freertos_kernel CMSDK_CM3_EXT)
