#!/bin/bash

# Find oldest NDK
if [ -n "$ANDROID_HOME" ] && [ -d "$ANDROID_HOME/ndk" ]; then
    # Get oldest NDK version
    OLDEST_NDK=$(find "$ANDROID_HOME/ndk" -maxdepth 1 -type d -name "*.*" 2>/dev/null | sort -V | head -1)

    if [ -n "$OLDEST_NDK" ] && [ -f "$OLDEST_NDK/build/cmake/android.toolchain.cmake" ]; then
        TOOLCHAIN="$OLDEST_NDK/build/cmake/android.toolchain.cmake"
        echo "Using NDK: $(basename "$OLDEST_NDK")"

        # Build all architectures
        for ARCH in "armeabi-v7a" "arm64-v8a" "x86" "x86_64"; do
            echo "Building $ARCH..."
            mkdir -p out
            cmake -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
                  -DANDROID_ABI="$ARCH" \
                  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                  -B "out/$ARCH" && \
            cmake --build "out/$ARCH"
        done
    else
        echo "No valid NDK found"
    fi
else
    echo "ANDROID_HOME not set or no NDK directory"
fi
