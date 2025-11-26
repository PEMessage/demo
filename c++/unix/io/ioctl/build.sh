#!/bin/bash

if [ -n "$ANDROID_HOME" ] && [ -f "$ANDROID_HOME/ndk/21.0.6113669/build/cmake/android.toolchain.cmake" ] ; then
    cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_HOME/ndk/21.0.6113669/build/cmake/android.toolchain.cmake -DANDROID_ABI=armeabi-v7a -B build_arm &&
        cmake --build build_arm
fi

cmake  -B build &&
    cmake --build build
