add_library(freertos_hook
    STATIC
        hook.c
    )
target_link_libraries(freertos_hook freertos_kernel freertos_config)
