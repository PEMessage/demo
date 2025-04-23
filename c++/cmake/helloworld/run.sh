#!/bin/bash
for f in *.cmake
do
    (
    set -x
    OUT="out/${f%*.cmake}"
    mkdir -p "$OUT"
    # -B <path-to-build>
    # -P <cmake-script-file>
    cmake -P "$f" -B "$OUT"
    )
done


