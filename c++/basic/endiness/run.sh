#!/bin/bash

# Check if main.c exists
if [ ! -f "main.c" ]; then
    echo "Error: main.c not found"
    exit 1
fi

# Check if zig is installed
if ! command -v zig >/dev/null 2>&1; then
    echo "Error: zig is not installed"
    exit 1
fi

echo "Compiling main.c for multiple architectures..."

# Test each architecture
# use `zig targets` to list all
architectures=(
    "x86_64-linux-musl qemu-x86_64"
    "aarch64-linux-musl qemu-aarch64"
    "arm-linux-musleabihf qemu-arm"
    "mips-linux-musleabi qemu-mips"
    "mipsel-linux-musleabi qemu-mipsel"
    "powerpc-linux-musleabi qemu-ppc"
    "riscv64-linux-musl qemu-riscv64"
)

build_dir="build"
mkdir -p "$build_dir"

cflags=("-Os" "-g")

for entry in "${architectures[@]}"; do
    target=$(echo "$entry" | cut -d' ' -f1)
    qemu=$(echo "$entry" | cut -d' ' -f2)
    arch_name="${target%%-*}"
    binary="$build_dir/main_${arch_name}.out"
    disasm_file="$build_dir/main_${arch_name}.asm"

    echo "=== $target ==="

    # Compile
    if zig cc -target "$target" main.c -o "$binary" "${cflags[@]}" ; then
        echo "Trying llvm-objdump ..."
        if command -v llvm-objdump > /dev/null; then
            llvm-objdump -S "$binary" > "$disasm_file" 2>/dev/null || true
        fi


        # Run with QEMU if available
        if command -v "$qemu" >/dev/null 2>&1; then
            "$qemu" "./$binary" 2>/dev/null
        fi
    fi
    echo ""
done
