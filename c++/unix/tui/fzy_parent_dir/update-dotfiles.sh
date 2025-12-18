#!/bin/sh
set -x
./docker-compile.sh
cp fzy_parent_dir $PEM_HOME/bin/arch/linux-x86_64/c.d/parent_dir_tui
