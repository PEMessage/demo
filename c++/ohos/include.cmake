set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "" FORCE)
set(CMAKE_BUILD_TYPE Debug CACHE INTERNAL "" FORCE)

set(CMAKE_C_FLAGS_DEBUG "-g3 -ggdb -O0" CACHE STRING "C Compiler Flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "C++ Compiler Flags" FORCE)

function(find_project_root out_var)
    # Start from current source directory
    set(current_dir "${CMAKE_CURRENT_SOURCE_DIR}")

    # Loop upwards through directories
    while(NOT "${current_dir}" STREQUAL "/")
        # Check if marker file exists
        if(EXISTS "${current_dir}/.project_root")
            set(${out_var} "${current_dir}" PARENT_SCOPE)
            return()
        endif()

        # Move up one directory
        get_filename_component(parent_dir "${current_dir}" DIRECTORY)
        if("${parent_dir}" STREQUAL "${current_dir}")
            # We've reached the root directory
            break()
        endif()
        set(current_dir "${parent_dir}")
    endwhile()

    # If we get here, no marker file was found
    message(WARNING "Could not find project root directory (no .project_root marker file found)")
    set(${out_var} "${CMAKE_SOURCE_DIR}" PARENT_SCOPE) # Fallback to default
endfunction()
find_project_root(PROJECT_ROOT)


macro(auto_target_link_libraries target visibility)
    # Parse arguments - all remaining args are treated as library names
    set(libnames ${ARGN})

    if(NOT libnames)
        message(FATAL_ERROR "No library names provided for target_link_libraries_withdir")
    endif()

    foreach(libname IN LISTS libnames)
        # Use find command to locate the library directory
        execute_process(
            COMMAND find "${PROJECT_ROOT}/lib" -name "${libname}" -type d
            OUTPUT_VARIABLE LIBDIRS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET  # Suppress error output if not found
        )

        # Split found directories and take first match
        string(REPLACE "\n" ";" LIBDIR_LIST ${LIBDIRS})

        if(NOT LIBDIR_LIST)
            message(FATAL_ERROR "Could not find library directory for: ${libname}")
        endif()

        list(GET LIBDIR_LIST 0 LIBDIR)

        message(STATUS "Found ${libname} at: ${LIBDIR}")

        # Check if we've already added this subdirectory
        get_property(subdirs_added GLOBAL PROPERTY AUTO_TARGET_LINK_LIBRARIES_ADDED_DIR)
        message("${subdirs_added}")
        if(NOT ${LIBDIR} IN_LIST subdirs_added)
            # Add subdirectory and link library
            add_subdirectory(${LIBDIR} ${libname})
            set_property(GLOBAL APPEND PROPERTY AUTO_TARGET_LINK_LIBRARIES_ADDED_DIR
                ${LIBDIR})
        endif()

        target_link_libraries(${target} ${visibility} ${libname})
    endforeach()
endmacro()

# Initialize the global property to track added subdirectories
define_property(GLOBAL PROPERTY AUTO_TARGET_LINK_LIBRARIES_ADDED_DIR
    BRIEF_DOCS "List of directories already added by target_link_libraries_withdir"
    FULL_DOCS "Tracks which library directories have already been added to avoid duplicates")
