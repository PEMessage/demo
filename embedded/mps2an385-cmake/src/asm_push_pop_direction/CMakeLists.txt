# SVC Handler Module
cmake_minimum_required(VERSION 3.12)
project(asm_push_pop_direction LANGUAGES C ASM)

# Set module name
set(MODULE_NAME asm_push_pop_direction)

# Source files
file(GLOB SRCS *.c)

# Create executable
add_executable_with_binaries(${MODULE_NAME} ${SRCS})

# Link libraries
target_link_libraries(${MODULE_NAME}
    CMSDK_CM3
    CMSIS
)

# Include directories
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
