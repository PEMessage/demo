#!/bin/bash


gcc -ggdb -g3 -nostartfiles -static minimal_starfiles.c -o minimal_starfiles.out &&
    ./minimal_starfiles.out "$@"
echo runing result is $?

