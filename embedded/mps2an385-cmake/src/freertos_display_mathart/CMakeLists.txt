# Hello World Module
cmake_minimum_required(VERSION 3.12)
project(freertos_display_mathart LANGUAGES C ASM)

# Set module name
set(MODULE_NAME freertos_display_mathart)

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
)

# Include directories
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
