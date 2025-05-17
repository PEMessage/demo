include(FetchContent)

# Fetch content using git clone
FetchContent_Declare(
    FreeRTOS-Kernel-Source
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG        V11.2.0
    SOURCE_DIR     ${CMAKE_CURRENT_LIST_DIR}/code
)

FetchContent_MakeAvailable(FreeRTOS-Kernel-Source)
