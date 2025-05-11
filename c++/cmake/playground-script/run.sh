#!/bin/bash

if [ ! -d "$1" ] ; then
    echo "[Err]: must given a dir"
    exit 0
fi


# Hook isatty, to ensure that cmake output color
if [ ! -f "fakeisatty/fakeisatty.so" ] ; then
    gcc -shared -fPIC -o fakeisatty/fakeisatty.so fakeisatty/fakeisatty.c
fi


for f in "$1"/*.cmake
do
    (
    set -x
    OUT="out/${f%*.cmake}"
    mkdir -p "$OUT"
    # -B <path-to-build>
    # -P <cmake-script-file>
    LD_PRELOAD="./fakeisatty/fakeisatty.so" \
    cmake --trace-source="$f" \
    -fdiagnostics-color=always \
    --force-color \
    -DCMAKE_COLOR_DIAGNOSTICS=ON \
    -P "$f" \
    -B "$OUT" |& sed "s@$(pwd)/$1@@g"
    )
done


