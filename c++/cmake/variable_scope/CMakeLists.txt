cmake_minimum_required(VERSION 3.10)
project(ScopeDemo)

message("\n=== Root CMakeLists.txt ===")
message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message("CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")

# First demonstrate include() behavior
message("\n=== Using include() ===")
include("include_vs_subdir/CMakeLists.txt")

# Then demonstrate add_subdirectory() behavior
message("\n=== Using add_subdirectory() ===")
add_subdirectory("include_vs_subdir")
