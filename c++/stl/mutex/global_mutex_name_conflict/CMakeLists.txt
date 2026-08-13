cmake_minimum_required(VERSION 3.16)
project(so_global_mutex_demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall)

find_package(Threads REQUIRED)

# Solution 2: instrument the host as well.
#   ON  -> host (main) also links libasan first in its DT_NEEDED,
#          no LD_PRELOAD needed.
#   OFF -> host is NOT ASan-instrumented; loading the instrumented .so
#          fails with "ASan runtime does not come first" unless you
#          LD_PRELOAD libasan (see run_all.sh).
option(ASAN_HOST "Build the host (main) with ASan too" OFF)

set(ASAN_FLAGS -fsanitize=address -fno-omit-frame-pointer)

function(make_variant variant)
    add_library(liba_${variant} SHARED liba.cpp)
    add_library(libb_${variant} SHARED libb.cpp)
    target_compile_options(liba_${variant} PRIVATE ${ASAN_FLAGS})
    target_compile_options(libb_${variant} PRIVATE ${ASAN_FLAGS})
    target_link_options(liba_${variant} PRIVATE -fsanitize=address)
    target_link_options(libb_${variant} PRIVATE -fsanitize=address)
    target_link_libraries(liba_${variant} PRIVATE Threads::Threads)
    target_link_libraries(libb_${variant} PRIVATE Threads::Threads)

    if(variant STREQUAL "static")
        target_compile_definitions(liba_${variant} PRIVATE MUTEX_STATIC)
        target_compile_definitions(libb_${variant} PRIVATE MUTEX_STATIC)
    elseif(variant STREQUAL "ns")
        target_compile_definitions(liba_${variant} PRIVATE MUTEX_NAMESPACE)
        target_compile_definitions(libb_${variant} PRIVATE MUTEX_NAMESPACE)
    elseif(variant STREQUAL "symbolic")
        # -Bsymbolic: bind references to our own definitions locally at link time,
        # so liba/libb each keep their own g_mutex (though g_mutex stays in .dynsym).
        target_link_options(liba_${variant} PRIVATE -Wl,-Bsymbolic)
        target_link_options(libb_${variant} PRIVATE -Wl,-Bsymbolic)
    elseif(variant STREQUAL "hidden")
        # -fvisibility=hidden + -DHIDDEN: only API functions (marked with the
        # visibility attribute) are exported; g_mutex stays hidden.
        target_compile_definitions(liba_${variant} PRIVATE HIDDEN)
        target_compile_definitions(libb_${variant} PRIVATE HIDDEN)
        target_compile_options(liba_${variant} PRIVATE -fvisibility=hidden)
        target_compile_options(libb_${variant} PRIVATE -fvisibility=hidden)
    elseif(variant STREQUAL "verscript")
        # version script: only a_say/b_say are exported, everything else (incl.
        # g_mutex) is local. Cleanest no-source-change fix.
        target_link_options(liba_${variant} PRIVATE
            -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/export.map)
        target_link_options(libb_${variant} PRIVATE
            -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/export.map)
    endif()

    add_executable(main_${variant} main.cpp)
    if(ASAN_HOST)
        # host linked with ASan: libasan becomes the FIRST DT_NEEDED entry
        target_compile_options(main_${variant} PRIVATE ${ASAN_FLAGS})
        target_link_options(main_${variant} PRIVATE -fsanitize=address)
    endif()
    target_link_libraries(main_${variant} PRIVATE liba_${variant} libb_${variant})
endfunction()

make_variant(plain)
make_variant(static)
make_variant(ns)
make_variant(symbolic)
make_variant(hidden)
make_variant(verscript)
