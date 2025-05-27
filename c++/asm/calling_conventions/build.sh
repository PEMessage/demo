#!/bin/bash

# Output directories
X86_64_BUILD="build/x86_64"
X86_BUILD="build/x86"

ARM64_BUILD="build/arm64"

RISCV64_BUILD="build/riscv64"
RISCV32_BUILD="build/riscv32"

# -nostartfiles: Don't use standard startup files (crt0, etc.)
# -nostdlib: Don't link with standard system libraries
CFLAGS="-ggdb -g3 -O0 -nostartfiles -nostdlib -static"

mkdir -p "$X86_64_BUILD" "$X86_BUILD" \
    "$ARM64_BUILD" "$ARMV7M_BUILD" \
    "$RISCV64_BUILD" "$RISCV32_BUILD" 

# x86_64 build (Intel syntax disassembly)
x86_64_build() {
    echo "Building x86_64..."
    gcc $CFLAGS caller_callee.c -o "$X86_64_BUILD/caller_callee"
    # -S : Mix source code with disassembly (requires debug info).
    # -M intel : Uses Intel syntax instead of AT&T.
    objdump -S "$X86_64_BUILD/caller_callee" -M intel > "$X86_64_BUILD/caller_callee.S"
}

x86_build() {
    echo "Building x86 (32-bit)..."
    gcc -m32 $CFLAGS caller_callee.c -o "$X86_BUILD/caller_callee"
    objdump -S "$X86_BUILD/caller_callee" -M intel > "$X86_BUILD/caller_callee.S"
}


arm64_build() {
    echo "Building ARM64..."
    aarch64-linux-gnu-gcc $CFLAGS caller_callee.c -o "$ARM64_BUILD/caller_callee"
    aarch64-linux-gnu-objdump -S "$ARM64_BUILD/caller_callee" > "$ARM64_BUILD/caller_callee.S"
}

riscv64_build() {
    echo "Building RISC-V 64-bit..."
    riscv64-linux-gnu-gcc $CFLAGS caller_callee.c -o "$RISCV64_BUILD/caller_callee"
    riscv64-linux-gnu-objdump -S "$RISCV64_BUILD/caller_callee" > "$RISCV64_BUILD/caller_callee.S"
}

# RISC-V 32-bit build
riscv32_build() {
    echo "Building RISC-V 32-bit..."
    riscv64-linux-gnu-gcc -march=rv32ima -mabi=ilp32 $CFLAGS caller_callee.c -o "$RISCV32_BUILD/caller_callee"
    riscv64-linux-gnu-objdump -S "$RISCV32_BUILD/caller_callee" > "$RISCV32_BUILD/caller_callee.S"
}


run_x86_64() {
    echo  "Running x86_64 binary..."
    "$X86_64_BUILD/caller_callee"
    echo "ret: $?"
}

run_x86() {
    echo "Running x86 (32-bit) binary..."
    "$X86_BUILD/caller_callee"
    echo "ret: $?"
}

# Run ARM64 binary with QEMU user-mode
run_arm64() {
    echo "Running ARM64 binary with qemu-system-aarch64..."
    qemu-aarch64-static "$ARM64_BUILD/caller_callee"
    echo "ret: $?"
}

run_riscv64() {
    echo "Running RISC-V 64-bit binary with qemu-riscv64..."
    qemu-riscv64-static "$RISCV64_BUILD/caller_callee"
    echo "ret: $?"
}

run_riscv32() {
    echo "Running RISC-V 32-bit binary with qemu-riscv32..."
    qemu-riscv32-static "$RISCV32_BUILD/caller_callee"
    echo "ret: $?"
}


# Build all targets
x86_64_build
x86_build
arm64_build
riscv64_build
riscv32_build


echo


run_x86_64
run_x86
run_arm64
run_riscv64
run_riscv32

echo

echo "Done! Check the 'build/' directory for outputs."
