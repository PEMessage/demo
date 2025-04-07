# ARM Cortex-M3 Toolchain Configuration
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Toolchain paths
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_LINKER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

# Workaround to make clangd happy
# set(GCC_INCLUDE_FLAGS "-isystem /usr/lib/arm-none-eabi/include/")

# Compiler flags
set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -Wall -Os -g -g3 ${GCC_INCLUDE_FLAGS} -nostdlib" CACHE STRING "C Compiler Flags" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "C++ Compiler Flags" FORCE)
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "ASM Compiler Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-nostartfiles" CACHE STRING "Linker Flags" FORCE)

# Disable compiler checks
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)


# add newlib
add_subdirectory(lib/newlib)


# global dep
add_library(global_dependencies INTERFACE)
target_link_libraries(global_dependencies INTERFACE
    newlib::newlib-nano
    arm_toolchain_objects
)

# toolchain hook, will be call by add_executable_with_binaries
macro(add_executable_toolchain_hook MODULE_NAME)
    target_link_libraries(${MODULE_NAME} global_dependencies)
endmacro()
