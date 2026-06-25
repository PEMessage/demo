# Build newlib-nano from source and provide it as an IMPORTED target.
# This is a subdirectory of the root project; no project() call needed.

cmake_minimum_required(VERSION 3.15)

include(FetchContent)
include(ExternalProject)

set(NEWLIB_TARGET_TRIPLET "arm-none-eabi" CACHE STRING "Cross-compilation target triplet")
# Must match NEWLIB_INSTALL_PREFIX in toolchain-arm-newlib.cmake
set(NEWLIB_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Self-built newlib install prefix")
set(NEWLIB_INCDIR "${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/include")
set(NEWLIB_LIBDIR "${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp")

# -------------------------------------------------------------------
# 1. Fetch newlib source (configure-time; download only, no add_subdirectory)
# -------------------------------------------------------------------
FetchContent_Declare(
    newlib_src
    GIT_REPOSITORY  git://sourceware.org/git/newlib-cygwin.git
    GIT_TAG         7923059bff6c120c6fb74b63c7553ea345c0a8f3
    SOURCE_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/newlib-src"
)

# Use FetchContent_Populate (not MakeAvailable) to avoid add_subdirectory
# on the autotools-based newlib source tree
FetchContent_GetProperties(newlib_src)
if(NOT newlib_src_POPULATED)
    FetchContent_Populate(newlib_src)
endif()

# -------------------------------------------------------------------
# 2. Build newlib at build-time via ExternalProject (configure + make + install)
#
#   Build flags reference (Arch Linux arm-none-eabi-newlib PKGBUILD):
#     https://gitlab.archlinux.org/archlinux/packaging/packages/arm-none-eabi-newlib/-/blob/main/PKGBUILD?ref_type=heads
#   Multilib notes:
#     Key is `arm-none-eabi-gcc --print-multi-lib`
#     config-ml.in: https://gcc.gnu.org/pipermail/gcc/2020-March/000218.html
# -------------------------------------------------------------------
set(COMMON_CONFIGURE_ARGS
    --target=${NEWLIB_TARGET_TRIPLET}
    --prefix=${NEWLIB_INSTALL_PREFIX}
)

# Target compilation flags: Cortex-M3 is Thumb-only, newlib must be compiled
# with -mthumb or the startup code (crt0.o) will be ARM code → HardFault.
set(NEWLIB_TARGET_CFLAGS "-mcpu=cortex-m3 -mthumb -Os -ffunction-sections -fdata-sections")

ExternalProject_Add(
    newlib-nano
    SOURCE_DIR      "${newlib_src_SOURCE_DIR}"
    BINARY_DIR      "${CMAKE_BINARY_DIR}/build-nano"
    CONFIGURE_COMMAND
        env CFLAGS_FOR_TARGET=${NEWLIB_TARGET_CFLAGS}
        <SOURCE_DIR>/configure
        ${COMMON_CONFIGURE_ARGS}
        --disable-nls
        --disable-newlib-supplied-syscalls
        --enable-newlib-reent-small
        --enable-newlib-retargetable-locking
        --disable-newlib-fvwrite-in-streamio
        --disable-newlib-fseek-optimization
        --disable-newlib-wide-orient
        --enable-newlib-nano-malloc
        --disable-newlib-unbuf-stream-opt
        --enable-lite-exit
        --enable-newlib-global-atexit
        --enable-newlib-nano-formatted-io
        --with-multisubdir=thumb/v7-m/nofp
    BUILD_COMMAND      ${CMAKE_MAKE_PROGRAM} CFLAGS_FOR_TARGET=${NEWLIB_TARGET_CFLAGS}
    INSTALL_COMMAND    ${CMAKE_MAKE_PROGRAM} install
    BUILD_BYPRODUCTS
        ${NEWLIB_LIBDIR}/libc.a
        ${NEWLIB_LIBDIR}/libm.a
        ${NEWLIB_LIBDIR}/libnosys.a
        ${NEWLIB_LIBDIR}/crt0.o
)

# -------------------------------------------------------------------
# 3. Locate GCC toolchain object files and libraries (configure-time)
# -------------------------------------------------------------------
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -mcpu=cortex-m3 -mthumb -print-file-name=crti.o
    OUTPUT_VARIABLE GCC_CRTI
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -mcpu=cortex-m3 -mthumb -print-file-name=crtn.o
    OUTPUT_VARIABLE GCC_CRTN
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -mcpu=cortex-m3 -mthumb -print-libgcc-file-name
    OUTPUT_VARIABLE GCC_LIBGCC
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "GCC crti:      ${GCC_CRTI}")
message(STATUS "GCC crtn:      ${GCC_CRTN}")
message(STATUS "GCC libgcc:    ${GCC_LIBGCC}")
message(STATUS "Newlib libdir: ${NEWLIB_LIBDIR}")

# -------------------------------------------------------------------
# 4. Bridge target: explicitly depends on the ExternalProject build step.
#    add_dependencies() does NOT support ExternalProject targets, so we
#    use a custom target as a bridge that regular targets can depend on.
# -------------------------------------------------------------------
add_custom_target(newlib_nano_ready DEPENDS newlib-nano)

# -------------------------------------------------------------------
# 5. Global dependencies with correct link order.
#
#    Link order matters for static archives:
#      crti.o  —_init prologue (GCC)
#      crt0.o  — _start entry point (self-built newlib)
#      libnosys.a, libm.a — depend on libc (must come before libc)
#      libc.a  — C library (self-built newlib)
#      libgcc.a — GCC runtime (division, FP helpers)
#      crtn.o  — _init epilogue (GCC)
# -------------------------------------------------------------------
add_library(global_dependencies INTERFACE)
target_link_libraries(global_dependencies INTERFACE
    ${GCC_CRTI}
    ${NEWLIB_LIBDIR}/crt0.o
    "-Wl,--start-group"
    ${NEWLIB_LIBDIR}/libm.a
    ${NEWLIB_LIBDIR}/libc.a
    ${NEWLIB_LIBDIR}/libnosys.a
    "-Wl,--end-group"
    ${GCC_LIBGCC}
    ${GCC_CRTN}
)
target_include_directories(global_dependencies INTERFACE ${NEWLIB_INCDIR})

# -------------------------------------------------------------------
# 6. Toolchain hook — auto-linked via add_executable_with_binaries()
# -------------------------------------------------------------------
macro(add_executable_toolchain_hook MODULE_NAME)
    target_link_libraries(${MODULE_NAME} global_dependencies)
    add_dependencies(${MODULE_NAME} newlib_nano_ready)
endmacro()
