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

# Add modules (built separately via their own CMakeLists)
option(BUILD_HELLO_WORLD "Build hello_world module" ON)
# option(BUILD_SVC_HANDLER "Build svc_handler module" OFF)

if(BUILD_HELLO_WORLD)
  add_subdirectory(src/hello_world)
endif()

# if(BUILD_SVC_HANDLER)
#   add_subdirectory(src/svc_handler)
# endif()
