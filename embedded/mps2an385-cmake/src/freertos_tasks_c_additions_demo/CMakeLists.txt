cmake_minimum_required(VERSION 3.12)
project(freertos_tasks_c_additions_demo LANGUAGES C ASM)

set(MODULE_NAME freertos_tasks_c_additions_demo)

file(GLOB SRCS *.c)

add_executable_with_binaries(${MODULE_NAME} ${SRCS})

target_link_libraries(${MODULE_NAME}
    CMSDK_CM3
    CMSIS
    freertos_kernel
    freertos_config
    freertos_hook
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
