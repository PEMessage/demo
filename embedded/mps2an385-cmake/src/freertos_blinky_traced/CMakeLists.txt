# Hello World Module
cmake_minimum_required(VERSION 3.12)
project(freertos_blinky_traced LANGUAGES C ASM)

# Set module name
set(MODULE_NAME freertos_blinky_traced)

# Source files
file(GLOB SRCS *.c)

# Create executable
add_executable_with_binaries(${MODULE_NAME} ${SRCS})

# Link libraries
target_link_libraries(${MODULE_NAME}
    CMSDK_CM3
    CMSIS
    freertos_kernel
    freertos_config_with_trace
    freertos_hook
)

# Include directories
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
