#!/usr/bin/env bash
# Scenario: the host (main) is NOT built with ASan; only liba/libb .so are
# instrumented with ASan. Build first, then run all three variants
# (plain / static / ns).
set -u
cd "$(dirname "$0")"

cmake -S . -B build
cmake --build build

# Why LD_PRELOAD is required:
#   Since the host does not link libasan, libasan is pulled in transitively
#   via the DT_NEEDED of liba/libb, i.e. it loads AFTER libstdc++/libc.
#   The ASan runtime then refuses to start and aborts with:
#     "ASan runtime does not come first in initial library list"
#   So we must LD_PRELOAD libasan to force it to the front of the load order.
#   Only then does the program run, and ASan can catch (at load time, via its
#   ODR check) the duplicate global `g_mutex` in the plain variant.
ASAN_LIB="$(g++ -print-file-name=libasan.so)"
[ -n "$ASAN_LIB" ] && export LD_PRELOAD="$ASAN_LIB"

for v in plain static ns symbolic hidden verscript; do
    bin="./build/main_$v"
    echo "========== main_$v =========="
    "$bin"
    echo "exit=$?"
    echo "-- liba_$v exported symbols (T: functions, B: global data) --"
    nm -D "./build/libliba_$v.so" | grep ' [TB] '
    echo
done
