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

# # Workaround to make clangd happy
# set(GCC_INCLUDE_FLAGS "-isystem /usr/lib/arm-none-eabi/include/")

# Update 20260922:
#   Thanks: https://discourse.nixos.org/t/get-clangd-to-find-standard-headers-in-nix-shell/11268
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

# Compiler flags
set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -Wall -Os -g -g3 ${GCC_INCLUDE_FLAGS} -ffunction-sections -fdata-sections" CACHE STRING "C Compiler Flags")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "C++ Compiler Flags")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "ASM Compiler Flags")
set(CMAKE_EXE_LINKER_FLAGS "-specs=nosys.specs -lnosys -Wl,--gc-sections" CACHE STRING "Linker Flags")

# Disable compiler checks
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
