# Hello World Module
cmake_minimum_required(VERSION 3.12)
project(hello_world_fstackprotector LANGUAGES C ASM)

# Set module name
set(MODULE_NAME hello_world_fstackprotector)

# Source files
file(GLOB SRCS *.c)

# Create executable
add_executable_with_binaries(${MODULE_NAME} ${SRCS})

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")
target_compile_options(${MODULE_NAME} PRIVATE -fstack-protector)
target_link_options(${MODULE_NAME} PUBLIC
    -Wl,--wrap=__stack_chk_fail
)

# Link libraries
target_link_libraries(${MODULE_NAME}
    CMSDK_CM3
    CMSIS
)

# Include directories
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
