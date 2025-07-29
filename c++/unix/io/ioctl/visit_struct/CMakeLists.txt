
include(FetchContent)

set(VISIT_STRUCT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/code")

if(NOT EXISTS "${VISIT_STRUCT_SOURCE_DIR}")
    FetchContent_Declare(
        visit_struct
        GIT_REPOSITORY https://github.com/cbeck88/visit_struct.git
        GIT_TAG 1.2.0
        SOURCE_DIR "${VISIT_STRUCT_SOURCE_DIR}"
    )
    message(STATUS "Fetching Result source into ${VISIT_STRUCT_SOURCE_DIR}")
    FetchContent_MakeAvailable(visit_struct)
else()
    message(STATUS "visit_struct already exists at ${VISIT_STRUCT_SOURCE_DIR}, skipping fetch.")
    add_subdirectory(${VISIT_STRUCT_SOURCE_DIR})
endif()

add_library(visit_struct INTERFACE)
target_include_directories(visit_struct INTERFACE code/include/)
