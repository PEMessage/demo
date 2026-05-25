#!/bin/bash

THISFILE="$(readlink -f "$0")"
THISDIR="$(dirname "$THISFILE")"
NAME="$(basename "$THISDIR")"

# -cp: --classpath
# java -cp "$THISDIR/build/classes/java/main" org.example.App "$@"

cd ../../
# ./gradlew "$NAME:run" "--args=\"$*\""
./gradlew "$NAME:run" --args="$*"
