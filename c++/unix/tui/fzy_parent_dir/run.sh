#!/bin/bash


mkdir -p fzy

version="34b88869d022e861da4846c4463aea3ddfb3ff30"

if [ ! -f "fzy/tty.c" ] ; then
    wget "https://raw.githubusercontent.com/jhawthorn/fzy/$version/src/tty.c" -O fzy/tty.c
fi

if [ ! -f "fzy/tty.h" ] ; then
    wget "https://raw.githubusercontent.com/jhawthorn/fzy/$version/src/tty.h" -O fzy/tty.h
fi

if [ ! -f "config.h" ] ; then
    wget "https://raw.githubusercontent.com/jhawthorn/fzy/$version/src/config.def.h" -O config.h
fi

gcc main.c fzy/tty.c -Ifzy -o fzy_parent_dir
./fzy_parent_dir "$@"
