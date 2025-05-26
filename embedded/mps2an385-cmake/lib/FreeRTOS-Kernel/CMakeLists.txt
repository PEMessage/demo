include(FetchContent)

# Fetch content using git clone
set(FREERTOS_KERNEL_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/code")

# Only declare and fetch if the source directory doesn't already exist
if(NOT EXISTS "${FREERTOS_KERNEL_SOURCE_DIR}")
  FetchContent_Declare(
    FreeRTOS-Kernel-Source
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG        V11.2.0
    SOURCE_DIR     "${FREERTOS_KERNEL_SOURCE_DIR}"
  )

  message(STATUS "Fetching FreeRTOS Kernel source into ${FREERTOS_KERNEL_SOURCE_DIR}")
  FetchContent_MakeAvailable(FreeRTOS-Kernel-Source)
else()
  message(STATUS "FreeRTOS Kernel already exists at ${FREERTOS_KERNEL_SOURCE_DIR}, skipping fetch.")
  add_subdirectory(${FREERTOS_KERNEL_SOURCE_DIR})
endif()
