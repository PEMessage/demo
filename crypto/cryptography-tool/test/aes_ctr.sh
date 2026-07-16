#!/usr/bin/env bash
# aes-ctr regression tests.
set -euo pipefail
cd "$(dirname "$0")/.."

fail=0
check() {
  local name=$1 expected=$2 actual=$3
  if [[ "$expected" == "$actual" ]]; then
    echo "PASS $name"
  else
    echo "FAIL $name"
    echo "  expected: $expected"
    echo "  actual:   $actual"
    fail=1
  fi
}

run() { uv run --script cryptography-tool -- "$@"; }

# --- aes-ctr: NIST SP 800-38A F.5.1 (CTR-AES128) ---
KEY=2b7e151628aed2a6abf7158809cf4f3c
NONCE=f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff
PT=6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710
CT=874d6191b620e3261bef6864990db6ce9806f66b7970fdff8617187bb9fffdff5ae4df3edbd5d35e5b4f09020db03eab1e031dda2fbe03d1792170a0f3009cee

check "aes-ctr encrypt (NIST F.5.1)" "$CT" \
  "$(run aes-ctr encrypt --key "$KEY" --nonce "$NONCE" --data "$PT")"
check "aes-ctr decrypt (NIST F.5.1)" "$PT" \
  "$(run aes-ctr decrypt --key "$KEY" --nonce "$NONCE" --data "$CT")"

# --- aes-ctr: NIST SP 800-38A F.5.5 (CTR-AES256) ---
KEY256=603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4
CT256=601ec313775789a5b7a7f504bbf3d228f443e3ca4d62b59aca84e990cacaf5c52b0930daa23de94ce87017ba2d84988ddfc9c58db67aada613c2dd08457941a6

check "aes-ctr encrypt (NIST F.5.5, AES-256)" "$CT256" \
  "$(run aes-ctr encrypt --key "$KEY256" --nonce "$NONCE" --data "$PT")"

# --- aes-ctr: partial final block + roundtrip ---
SHORT=00112233445566778899aabbccddeeff0102
ENC="$(run aes-ctr encrypt --key "$KEY" --nonce "$NONCE" --data "$SHORT")"
check "aes-ctr partial-block roundtrip" "$SHORT" \
  "$(run aes-ctr decrypt --key "$KEY" --nonce "$NONCE" --data "$ENC")"

exit "$fail"
