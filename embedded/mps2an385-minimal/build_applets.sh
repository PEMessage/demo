#!/bin/bash
set -e

CROSS_COMPILE=arm-none-eabi-
CC=${CROSS_COMPILE}gcc
OBJCOPY=${CROSS_COMPILE}objcopy
OBJDUMP=${CROSS_COMPILE}objdump

CFLAGS="-mcpu=cortex-m3 -mthumb -Wall -Os -fPIC -nostdlib -nostartfiles -fno-builtin"
LDFLAGS="-T applet_lib/applet.lds"

mkdir -p build/applets

echo "Building applets..."

for applet in applets/*.c; do
    name=$(basename "$applet" .c)
    echo "  Building $name..."
    
    $CC $CFLAGS -I applet_lib -c "$applet" -o "build/applets/${name}.o"
    $CC $CFLAGS $LDFLAGS "build/applets/${name}.o" -o "build/applets/${name}.elf"
    $OBJCOPY -O binary "build/applets/${name}.elf" "build/applets/${name}.bin"
    
    echo "  $name: $(stat -c%s build/applets/${name}.bin) bytes"
done

echo "Applet build complete."
