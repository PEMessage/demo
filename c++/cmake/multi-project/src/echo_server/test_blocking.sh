#!/usr/bin/env bash
set -euo pipefail

SERVER_BIN="${SERVER_BIN:-./build/src/echo_server/echo_server}"
PORT=7000

cleanup() {
  kill "$server_pid" 2>/dev/null || true
  kill "$client1_pid" 2>/dev/null || true
  kill "$client2_pid" 2>/dev/null || true
  wait 2>/dev/null || true
}
trap cleanup EXIT

echo "=== blocking echo server: two clients at the same time ==="
echo

"$SERVER_BIN" blocking &
server_pid=$!
sleep 0.5

tmp1=$(mktemp -u)
tmp2=$(mktemp -u)
mkfifo "$tmp1" "$tmp2"

# Client 1: sends 3 lines with 1s delay between each
( printf 'C1:a\n'; sleep 1; printf 'C1:b\n'; sleep 1; printf 'C1:c\n'; sleep 1 ) | nc -N localhost "$PORT" > "$tmp1" &
client1_pid=$!

sleep 0.2

# Client 2: sends 3 lines with 1s delay between each
( printf 'C2:x\n'; sleep 1; printf 'C2:y\n'; sleep 1; printf 'C2:z\n'; sleep 1 ) | nc -N localhost "$PORT" > "$tmp2" &
client2_pid=$!

echo "[C1 output]"
cat "$tmp1" &
echo "[C2 output]"
cat "$tmp2" &

sleep 5
echo
echo "=== done (C2 was blocked until C1 finished) ==="
