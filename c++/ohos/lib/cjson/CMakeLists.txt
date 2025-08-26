include(FetchContent)

set(CJSON_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/code")

if(NOT EXISTS "${CJSON_SOURCE_DIR}")
    FetchContent_Declare(
        cjson
        GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
        GIT_TAG v1.7.18
        SOURCE_DIR "${CJSON_SOURCE_DIR}"
    )
    message(STATUS "Fetching cjson source into ${CJSON_SOURCE_DIR}")
    FetchContent_MakeAvailable(cjson)
else()
    message(STATUS "cjson already exists at ${CJSON_SOURCE_DIR}, skipping fetch.")
    add_subdirectory(${CJSON_SOURCE_DIR})
endif()


target_include_directories(cjson PUBLIC code)
