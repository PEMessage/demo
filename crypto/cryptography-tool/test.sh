#!/usr/bin/env bash
# Quick regression suite: runs every test/*.sh module.
set -euo pipefail
cd "$(dirname "$0")"

fail=0
for t in test/*.sh; do
  echo "== $t =="
  bash "$t" || fail=1
done
exit "$fail"
