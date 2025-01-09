#!/bin/bash

build_download() {
    (
    if [ ! -d linux/.git ] ; then 
        git clone https://github.com/torvalds/linux.git -b v5.15 --depth=1
    fi
    )
}
# sudo apt install flex gcc make bison
# sudo apt install libelf-dev libssl-dev

build_config() {
    (
    cd linux
    mkdir -p build 
    make \
        O=build \
        defconfig
    # Order is important here. Must be first base config, then fragment
    # See:
    # https://stackoverflow.com/questions/7505164/how-do-you-non-interactively-turn-on-features-in-a-linux-kernel-config-file
    ./scripts/kconfig/merge_config.sh -O build build/.config ../config-fragment
    )
}

build_kernel() {
    (
    cd linux
    make O=build -j4
    )
}


# build_config
build_kernel
