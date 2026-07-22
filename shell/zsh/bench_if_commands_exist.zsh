#!/usr/bin/env zsh

setopt localoptions extendedglob
zmodload zsh/datetime 2>/dev/null

cmds=(ls cat sed awk grep python3 nonexistent_cmd_12345)
ITER=100000

echo "=========================================================="
echo "  Method 1: (( \$+commands[\$cmd] ))      — arithmetic"
echo "  Method 2: [[ -n \"\$commands[\$cmd]\" ]]   — [[ conditional"
echo "  Method 3: [ -n \"\$commands[\$cmd]\" ]     — [  builtin"
echo "=========================================================="
echo ""

bench() {
  local desc=$1 kind=$2
  echo "--- $desc ---"
  local total_ms=0
  for cmd in $cmds; do
    local sum=0
    for ((round=0; round<5; round++)); do
      local start=$EPOCHREALTIME
      for ((i=0; i<ITER; i++)); do
        case $kind in
          1) if (( $+commands[$cmd] )); then : ; fi ;;
          2) if [[ -n "$commands[$cmd]" ]]; then : ; fi ;;
          3) if [ -n "$commands[$cmd]" ]; then : ; fi ;;
        esac
      done
      local elapsed=$(( ($EPOCHREALTIME - $start) * 1000.0 ))
      sum=$(( sum + elapsed ))
    done
    local avg=$( printf "%.3f" $(( sum / 5.0 )) )
    printf "  %-30s avg %8s ms  (exists: %s)\n" "$cmd" "$avg" "$(( $+commands[$cmd] ))"
    total_ms=$(( total_ms + avg ))
  done
  printf "  %-30s   %8.3f ms\n" "SUM" "$total_ms"
  echo ""
}

bench "Method 1: (( \$+commands[\$cmd] ))"     1
bench "Method 2: [[ -n \"\$commands[\$cmd]\" ]]" 2
bench "Method 3: [  -n \"\$commands[\$cmd]\" ]"  3

echo "Done. ${ITER} iterations per command, averaged over 5 rounds."
