#!/bin/bash


./delombok `readlink -f src` -d `readlink -f src-gen`
