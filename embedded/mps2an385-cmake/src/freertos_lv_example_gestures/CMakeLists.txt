# Hello World Module
cmake_minimum_required(VERSION 3.12)
project(freertos_lv_example_gestures LANGUAGES C ASM)

# ===========================================================================
# NOTICE: you may need to set LV_MEM_SIZE 1024*1024 to make this example work
# ===========================================================================

# Set module name
set(MODULE_NAME freertos_lv_example_gestures)

# Source files
file(GLOB SRCS *.c)

# Create executable
add_executable_with_binaries(${MODULE_NAME} ${SRCS})


target_compile_options(${MODULE_NAME} PUBLIC )
# Link libraries
target_link_libraries(${MODULE_NAME}
    CMSDK_CM3
    CMSDK_CM3_EXT
    CMSIS
    freertos_kernel
    freertos_config
    freertos_hook
    m
    lvgl_port_with_multitouch
    lvgl
)

# Include directories
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
