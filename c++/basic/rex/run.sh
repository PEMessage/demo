#!/bin/bash

if [ ! -f nob.h ] ; then
    wget https://raw.githubusercontent.com/tsoding/nob.h/7deb15dcdbcb113794b79c60aabea6bada50aa93/nob.h
fi


gcc main.c -o main.out -I. -DNOB_STRIP_PREFIX -DNOB_IMPLEMENTATION
./main.out "$@"
