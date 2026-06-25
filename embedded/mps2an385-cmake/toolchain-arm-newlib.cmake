# ARM Cortex-M3 Toolchain with self-built newlib
# This toolchain file sets up the cross-compiler and flags.
# The newlib subdirectory (lib/newlib) is added by the root CMakeLists.txt
# after project(), where it builds newlib from source via ExternalProject_Add
# and creates IMPORTED targets.

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

# Disable compiler checks (they can't link without our custom newlib)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Path where self-built newlib will be installed (set here so we can use it
# for -isystem; must match NEWLIB_INSTALL_PREFIX in lib/newlib/CMakeLists.txt)
set(NEWLIB_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH
    "Install prefix for self-built newlib")

# Compiler flags
set(CMAKE_C_FLAGS
    "-mcpu=cortex-m3 -mthumb -Wall -Os -g -g3 -ffunction-sections -fdata-sections"
    CACHE STRING "C Compiler Flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "C++ Compiler Flags")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "ASM Compiler Flags")

# Linker: -nostdlib because we provide our own newlib, startup files, and libgcc
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,--gc-sections" CACHE STRING "Linker Flags")
