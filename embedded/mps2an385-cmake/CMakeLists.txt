# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.12)

# Include toolchain file
# if(DEFINED CMAKE_TOOLCHAIN_FILE)
include(${CMAKE_SOURCE_DIR}/toolchain-arm.cmake)
# endif()

# Project name (doesn't affect embedded builds)
project(mps2an385-minimal LANGUAGES C ASM)

# Add libraries
add_subdirectory(lib/CMSDK_CM3)
add_subdirectory(lib/CMSIS)
add_subdirectory(lib/FreeRTOS-Kernel-Config)
add_subdirectory(lib/FreeRTOS-Hook)
add_subdirectory(lib/FreeRTOS-Kernel)
add_subdirectory(lib/LVGL)

# Credit: deepseek-v3-0324
macro(add_executable_with_binaries MODULE_NAME)
    add_executable(${MODULE_NAME} ${ARGN})
    if(COMMAND add_executable_toolchain_hook)
        add_executable_toolchain_hook(${MODULE_NAME})
    endif()
    add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${MODULE_NAME}> ${MODULE_NAME}.bin
        COMMAND ${CMAKE_OBJDUMP} -S $<TARGET_FILE:${MODULE_NAME}> > ${MODULE_NAME}.lss
        COMMAND ln -sf $<TARGET_FILE:${MODULE_NAME}> ${CMAKE_BINARY_DIR}
        COMMENT "Generating binary and listing files for ${MODULE_NAME}"
    )
endmacro()

# add_subdirectory(src/hello_world)
# add_subdirectory(src/svc_handler)
# add_subdirectory(src/hello_world_systick)
# add_subdirectory(src/freertos_blinky)
# add_subdirectory(src/freertos_overflow)
# add_subdirectory(src/freertos_stackgrow_mallocgrow)

# Iterate through all directories under src/ and add them as subdirectories.
# This assumes each directory under src/ is a CMake project.
# This is more robust and avoids manually listing each subdirectory.

file(GLOB subdirectories src/*)
message("${subdirectories}")

foreach(subdir ${subdirectories})
  if(IS_DIRECTORY "${subdir}")
    add_subdirectory(${subdir})
  endif()
endforeach()
