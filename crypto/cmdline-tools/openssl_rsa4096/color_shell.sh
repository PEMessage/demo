#!/bin/bash
start=$(printf "\033[36m")
end=$(printf "\033[0m")
bash "$@" 2>&1 | awk -v start="$start" -v end="$end" '{print start $0 end}'
