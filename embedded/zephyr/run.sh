#!/bin/bash

set -ex

WEST='uvx --with jsonschema --with pyelftools west'


$WEST build -b mps2/an385 "$1"
qemu-system-arm -machine mps2-an385 -kernel build/zephyr/zephyr.bin -serial stdio -d cpu_reset
