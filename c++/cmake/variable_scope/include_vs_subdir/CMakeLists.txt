# This can be both included and used as a subdirectory
message("\n[Processing include_vs_subdir/CMakeLists.txt]")
message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message("CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")

# Add a simple target when used as subdirectory
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    message("This file is being processed in its own scope (add_subdirectory)")
else()
    message("This file is being included (include())")
endif()
