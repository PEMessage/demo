#!/bin/bash


if [ ! -f "$1" ] ; then
    echo "[Err]: must be a file"
fi


filename="$(basename "$1")"
binname="${filename%*.rs}"

declare -p binname

cargo run --bin "$binname"
