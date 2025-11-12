add_library(freertos_config INTERFACE)
add_library(freertos_config_with_trace INTERFACE)

target_include_directories(freertos_config SYSTEM INTERFACE
    include
    TraceRecorderConfig
)

target_include_directories(freertos_config_with_trace SYSTEM INTERFACE
    TraceRecorderConfig
)

target_link_libraries(freertos_config INTERFACE CMSDK_CM3)
target_link_libraries(freertos_config_with_trace INTERFACE freertos_config TraceRecorder)

target_compile_definitions(freertos_config
  INTERFACE
  projCOVERAGE_TEST=0
)

target_compile_definitions(freertos_config_with_trace
  INTERFACE
  projCOVERAGE_TEST=0
  ENABLE_TRACE_RECORDER=1
)

# cache as global var for target:freertos
set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set(FREERTOS_PORT "GCC_ARM_CM3" CACHE STRING "" FORCE)
# set(FREERTOS_PORT "GCC_ARM_CM3_MPU" CACHE STRING "" FORCE)

