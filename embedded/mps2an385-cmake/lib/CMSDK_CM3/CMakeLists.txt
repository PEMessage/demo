# CMSDK_CM3 Library
cmake_minimum_required(VERSION 3.12)
project(CMSDK_CM3 LANGUAGES C ASM)

# Source files
set(SRCS
    Source/hard_fault.c
    Source/startup_CMSDK_CM3.c
    Source/system_CMSDK_CM3.c
    Source/stdout_USART.c
    Source/USART_V2M-MPS2.c
)

# Include directories

# Create library
add_library(CMSDK_CM3 STATIC ${SRCS})

target_include_directories(CMSDK_CM3
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(CMSDK_CM3 CMSIS)

# Linker script
set_target_properties(CMSDK_CM3 PROPERTIES
    LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/gcc_arm.ld
)

target_link_options(CMSDK_CM3 INTERFACE
    -T ${CMAKE_SOURCE_DIR}/lib/CMSDK_CM3/Source/gcc_arm.ld
    --entry=Reset_Handler
    -Wl,-Map=gcc_arm.map # allow us to check gcc_arm.ld work fine
    -Wl,--print-memory-usage # also print usage
)

# Compile definitions
target_compile_definitions(CMSDK_CM3 PRIVATE CMSDK_CM3)
