include(FetchContent)

# Fetch content using git clone
set(TRACERECORDER_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/code")

# Only declare and fetch if the source directory doesn't already exist
if(NOT EXISTS "${TRACERECORDER_SOURCE_DIR}")
  FetchContent_Declare(
    TraceRecorderSource
    GIT_REPOSITORY https://github.com/percepio/TraceRecorderSource.git
    GIT_TAG        Tz4/4.10/v4.10.3
    SOURCE_DIR     "${TRACERECORDER_SOURCE_DIR}"
  )

  message(STATUS "Fetching tracerecorder source into ${TRACERECORDER_SOURCE_DIR}")
  FetchContent_MakeAvailable(TraceRecorderSource)
else()
  message(STATUS "tracerecorder already exists at ${TRACERECORDER_SOURCE_DIR}, skipping fetch.")
  # add_subdirectory(${TRACERECORDER_SOURCE_DIR})
endif()


# See: FreeRTOS/Demo/CORTEX_MPS2_QEMU_IAR_GCC/build/gcc/Makefile
set(TRACERECORDER_SOURCES
    ${TRACERECORDER_SOURCE_DIR}/kernelports/FreeRTOS/trcKernelPort.c
    ${TRACERECORDER_SOURCE_DIR}/streamports/RingBuffer/trcStreamPort.c
    ${TRACERECORDER_SOURCE_DIR}/trcAssert.c
    ${TRACERECORDER_SOURCE_DIR}/trcCounter.c
    ${TRACERECORDER_SOURCE_DIR}/trcDependency.c
    ${TRACERECORDER_SOURCE_DIR}/trcDiagnostics.c
    ${TRACERECORDER_SOURCE_DIR}/trcEntryTable.c
    ${TRACERECORDER_SOURCE_DIR}/trcError.c
    ${TRACERECORDER_SOURCE_DIR}/trcEvent.c
    ${TRACERECORDER_SOURCE_DIR}/trcEventBuffer.c
    ${TRACERECORDER_SOURCE_DIR}/trcExtension.c
    ${TRACERECORDER_SOURCE_DIR}/trcHardwarePort.c
    ${TRACERECORDER_SOURCE_DIR}/trcHeap.c
    ${TRACERECORDER_SOURCE_DIR}/trcInternalEventBuffer.c
    ${TRACERECORDER_SOURCE_DIR}/trcInterval.c
    ${TRACERECORDER_SOURCE_DIR}/trcISR.c
    ${TRACERECORDER_SOURCE_DIR}/trcMultiCoreEventBuffer.c
    ${TRACERECORDER_SOURCE_DIR}/trcObject.c
    ${TRACERECORDER_SOURCE_DIR}/trcPrint.c
    ${TRACERECORDER_SOURCE_DIR}/trcRunnable.c
    ${TRACERECORDER_SOURCE_DIR}/trcSnapshotRecorder.c
    ${TRACERECORDER_SOURCE_DIR}/trcStackMonitor.c
    ${TRACERECORDER_SOURCE_DIR}/trcStateMachine.c
    ${TRACERECORDER_SOURCE_DIR}/trcStaticBuffer.c
    ${TRACERECORDER_SOURCE_DIR}/trcStreamingRecorder.c
    ${TRACERECORDER_SOURCE_DIR}/trcString.c
    ${TRACERECORDER_SOURCE_DIR}/trcTask.c
    ${TRACERECORDER_SOURCE_DIR}/trcTimestamp.c
)

add_library(TraceRecorder ${TRACERECORDER_SOURCES})

# Add include directories
target_include_directories(TraceRecorder PUBLIC
    ${TRACERECORDER_SOURCE_DIR}/kernelports/FreeRTOS/include
    ${TRACERECORDER_SOURCE_DIR}/streamports/RingBuffer/include
    ${TRACERECORDER_SOURCE_DIR}/include
)

target_link_libraries(TraceRecorder PUBLIC freertos_kernel freertos_config CMSIS)
