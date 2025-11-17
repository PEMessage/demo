# CMSDK_CM3 Library
cmake_minimum_required(VERSION 3.12)
project(CMSDK_CM3 LANGUAGES C ASM)

# Source files
set(SRCS
    Source/startup_CMSDK_CM3.c
    Source/system_CMSDK_CM3.c
    Source/stdout_USART.c

    Source/Driver/UART/USART_V2M-MPS2.c

    Source/Driver/I2C/Driver_I2C.c
    Source/Driver/I2C/i2c_sbcon_drv.c
    Source/Driver/I2C/device_definition.c
)

# See: https://jefflongo.dev/posts/cmake-weak-symbol-oddities/
# Put strong symbol in obj, and wrap it in INTERFACE library
# ------
# 1. use object library
# ------
add_library(obj_hard_fault OBJECT Source/hard_fault.c)
    # This case only part
    target_include_directories(obj_hard_fault PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Include) # this case only
    target_link_libraries(obj_hard_fault PRIVATE CMSIS)
# ------
# 2. wrap in interface,
# Interface libraries do not compile sources, and do not produce output libraries, but can track dependencies and have properties
# ------
add_library(iface_hard_fault INTERFACE)
target_link_libraries(iface_hard_fault INTERFACE obj_hard_fault $<TARGET_OBJECTS:obj_hard_fault>)
# ------

# Include directories

# Create library
add_library(CMSDK_CM3 STATIC ${SRCS})

target_include_directories(CMSDK_CM3
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/Driver/
)

target_link_libraries(CMSDK_CM3 PRIVATE iface_hard_fault)
target_link_libraries(CMSDK_CM3 PUBLIC CMSIS)

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
