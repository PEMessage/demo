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

# Applet build flags.
#
#   -fPIC -shared        applet is position-independent with a PLT/GOT, so the
#                        loader can rebind its symbol references at load time.
#   -nostdlib -nostartfiles -fno-builtin
#                        no libc/startup inside the applet; printf/malloc/...
#                        stay UNDEFINED and are resolved by the loader against
#                        the host.
#   -fvisibility=hidden  make the DEFAULT symbol visibility "hidden", so only
#                        symbols explicitly marked with APPLET_EXPORT
#                        (= __attribute__((visibility("default")))) are exported.
#                        This noticeably shrinks the applet:
#                          * fewer dynamic symbols  -> smaller .dynsym/.dynstr/.hash
#                          * hidden globals are addressed directly (not via GOT)
#                            -> fewer .got slots + GLOB_DAT relocations
#                          * hidden functions are called directly (not via PLT)
#                            -> fewer .plt entries + JUMP_SLOT relocations
#                        Our applets need to export nothing (the loader finds
#                        the header by its magic at offset 0), so hiding
#                        everything is fine. If an applet must expose a symbol
#                        to ANOTHER applet, mark it with APPLET_EXPORT.
#                        (Linker-script symbols like __bss_start are NOT
#                        affected; hide those with PROVIDE_HIDDEN in applet.ld.)
CFLAGS="-mcpu=cortex-m3 -mthumb -Wall -Os -fPIC -shared -nostdlib -nostartfiles -fno-builtin -fvisibility=hidden"
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
