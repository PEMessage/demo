add_library(lvgl_port
    STATIC
        lv_port_disp.c
        lv_port_indev.c
    )
target_include_directories(lvgl_port PUBLIC
    ./
)
target_link_libraries(lvgl_port PRIVATE lvgl freertos_kernel CMSDK_CM3_EXT)
