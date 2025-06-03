add_library(freertos_config INTERFACE)

target_include_directories(freertos_config SYSTEM
    INTERFACE
    include
)

target_link_libraries(freertos_config INTERFACE CMSDK_CM3)

target_compile_definitions(freertos_config
  INTERFACE
  projCOVERAGE_TEST=0
)

# cache as global var for target:freertos
set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set(FREERTOS_PORT "GCC_ARM_CM3" CACHE STRING "" FORCE)
# set(FREERTOS_PORT "GCC_ARM_CM3_MPU" CACHE STRING "" FORCE)

