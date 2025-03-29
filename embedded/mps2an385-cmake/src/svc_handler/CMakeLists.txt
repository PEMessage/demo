# SVC Handler Module
cmake_minimum_required(VERSION 3.12)
project(svc_handler LANGUAGES C ASM)

# Set module name
set(MODULE_NAME svc_handler)

# Source files
file(GLOB SRCS *.c)

# Create executable
add_executable(${MODULE_NAME} ${SRCS})

# Link libraries
target_link_libraries(${MODULE_NAME}
    CMSDK_CM3
    CMSIS
)

# Include directories
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Linker script and flags
target_link_options(${MODULE_NAME} PRIVATE
    -T ${CMAKE_SOURCE_DIR}/lib/CMSDK_CM3/Source/gcc_arm.ld
    --entry=Reset_Handler
)

# Output files
set(BIN_FILE ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.bin)
set(LSS_FILE ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.lss)

# Custom commands for binary and listing
add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${MODULE_NAME}> ${BIN_FILE}
    COMMAND ${CMAKE_OBJDUMP} -S $<TARGET_FILE:${MODULE_NAME}> > ${LSS_FILE}
    COMMENT "Generating binary and listing files"
)

# QEMU targets
add_custom_target(run
    COMMAND qemu-system-arm
        -machine mps2-an385
        -cpu cortex-m3
        -kernel ${BIN_FILE}
        -monitor none
        -nographic -serial stdio
    DEPENDS ${MODULE_NAME}
)

add_custom_target(debug
    COMMAND qemu-system-arm
        -machine mps2-an385
        -cpu cortex-m3
        -kernel ${BIN_FILE}
        -monitor none
        -nographic -serial stdio
        -s -S
    DEPENDS ${MODULE_NAME}
)
