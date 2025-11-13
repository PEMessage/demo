#!/bin/bash
set -x

if [ ! -f "magic_enum.hpp" ] ; then
    wget https://raw.githubusercontent.com/Neargye/magic_enum/refs/heads/master/include/magic_enum/magic_enum.hpp
fi

g++ -o "event_spy.out" "event_spy.cpp" \
    -ggdb -g3 -fsanitize=address -Wall \
    -I. \
    `pkg-config --cflags --libs gtkmm-3.0`

./event_spy.out
