#!/bin/bash
# cmake -DCMAKE_TOOLCHAIN_FILE=/opt/android-sdk/ndk/21.0.6113669/build/cmake/android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -B build_arm &&
#     cmake --build build_arm

cmake  -B build &&
    cmake --build build
