cmake_minimum_required(VERSION 3.15)
project(newlib-nano LANGUAGES C)

include(FetchContent)
include(ExternalProject)

set(NEWLIB_TARGET_TRIPLET "arm-none-eabi" CACHE STRING "Target triplet for cross-compilation")
set(NEWLIB_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation prefix for newlib-nano")

FetchContent_Declare(
    newlib_src
    GIT_REPOSITORY git://sourceware.org/git/newlib-cygwin.git
    GIT_TAG 7923059bff6c120c6fb74b63c7553ea345c0a8f3
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/newlib-src"
)
FetchContent_MakeAvailable(newlib_src)

set(COMMON_CONFIGURE_ARGS
    --target=${NEWLIB_TARGET_TRIPLET}
    --prefix=${NEWLIB_INSTALL_PREFIX}
)

# How to build, please check this to build newlib-nano
#   https://gitlab.archlinux.org/archlinux/packaging/packages/arm-none-eabi-newlib/-/blob/main/PKGBUILD?ref_type=heads
# Only build v7-m:
#   Key is `gcc arm-none-eabi-gcc --print-multi-lib`
#   and config-ml.in
# Also see:
#   https://gcc.gnu.org/pipermail/gcc/2020-March/000218.html
ExternalProject_Add(
    newlib-nano
    SOURCE_DIR "${newlib_src_SOURCE_DIR}"
    BINARY_DIR "${CMAKE_BINARY_DIR}/build-nano"
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
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
    BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
    INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install
    BUILD_BYPRODUCTS
        ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libm.a
        ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libc.a
        ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libg.a
        ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libnosys.a
        # ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/librdimon.a
        # ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/librdpmon.a
        # ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/librdimon-v2m.a
)

add_library(newlib::newlib-nano OBJECT IMPORTED GLOBAL)
# target_include_directories(newlib::newlib-nano INTERFACE ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/include)
set_target_properties(newlib::newlib-nano PROPERTIES IMPORTED_OBJECTS
    ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libm.a
    ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libc.a
    ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libg.a
    ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/crt0.o
    ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libnosys.a
)
# target_link_libraries(newlib::newlib-nano INTERFACE
#     ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libm.a
#     ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libc.a
#     ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libg.a
#     ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/crt0.o
#     ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/libnosys.a
#     # ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/librdimon.a
#     # ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/librdpmon.a
#     # ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/librdimon-v2m.a
# )
add_dependencies(newlib::newlib-nano newlib-nano)
target_link_options(newlib::newlib-nano INTERFACE 
    -L${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp
    -lnosys -lc
    ${NEWLIB_INSTALL_PREFIX}/arm-none-eabi/lib/thumb/v7-m/nofp/crt0.o
)


# Find all .o files in the GCC ARM toolchain directory
file(GLOB ARM_TOOLCHAIN_OBJS "/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7-m/nofp/*.o")
add_library(arm_toolchain_objects OBJECT IMPORTED GLOBAL
    /usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7-m/nofp/crti.o
    )
set_property(TARGET arm_toolchain_objects PROPERTY IMPORTED_OBJECTS
  "/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7-m/nofp/crti.o"
)
# target_link_options(newlib::newlib-nano INTERFACE 
#     -L/usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7-m/nofp/
#     -lgcc
#     /usr/lib/gcc/arm-none-eabi/13.2.1/thumb/v7-m/nofp/crti.o
# )
# target_link_options(arm_toolchain_objects INTERFACE ${ARM_TOOLCHAIN_OBJS})
# set_property(
#     TARGET arm_toolchain_objects
#     PROPERTY IMPORTED_OBJECTS "${ARM_TOOLCHAIN_OBJS}"
# )

