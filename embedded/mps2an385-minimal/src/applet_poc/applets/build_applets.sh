#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Script directory: $SCRIPT_DIR"

runcmd() {
    echo "Runing: $*"
    "$@"
}

ELF="$1"
NAME="$(basename "$ELF" .elf)"
OBJ_DIR="$(dirname $ELF)/$NAME.d"
shift

SRCS="$@"


CROSS_COMPILE=arm-none-eabi-
CC=${CROSS_COMPILE}gcc
OBJCOPY=${CROSS_COMPILE}objcopy
OBJDUMP=${CROSS_COMPILE}objdump

CFLAGS="-mcpu=cortex-m3 -mthumb -Wall -Os -fPIC -nostdlib -nostartfiles -fno-builtin"
CFLAGS+=" -I ${SCRIPT_DIR}/applet_lib"
LDFLAGS="-T ${SCRIPT_DIR}/applet_lib/applet.ld"

prepare() {
    runcmd mkdir -p "$OBJ_DIR"
}


objs=""
build_obj() {
    for src in ${SRCS}; do
        local obj_name=$(basename "$src" .c)
        local obj="$OBJ_DIR/${obj_name}.o"
        runcmd $CC $CFLAGS -c "$src" -o "$obj"
        objs+=" $obj"
    done
}

prepare
build_obj
runcmd $CC $CFLAGS $LDFLAGS $objs -o "$ELF"
