
# set(LV_CONF_INCLUDE_SIMPLE OFF CACHE BOOL "Use simple include for lv_conf.h" FORCE)

set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE PATH
    "Use this to specify the location of and/or filename of lv_conf.h" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Build demos" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)

include(FetchContent)

# Fetch content using git clone
set(LVGL_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/code")

# Only declare and fetch if the source directory doesn't already exist
if(NOT EXISTS "${LVGL_SOURCE_DIR}")
  FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.3.0
    SOURCE_DIR     "${LVGL_SOURCE_DIR}"
  )

  message(STATUS "Fetching LVGL source into ${LVGL_SOURCE_DIR}")
  FetchContent_MakeAvailable(lvgl)
else()
  message(STATUS "LVGL already exists at ${LVGL_SOURCE_DIR}, skipping fetch.")
  add_subdirectory(${LVGL_SOURCE_DIR})
endif()

add_subdirectory(lvgl_port)

cmake_policy(SET CMP0079 NEW) # inject freertos_kernel into lvgl dep
target_link_libraries(lvgl PRIVATE freertos_kernel m)

