#!/usr/bin/env bash
# hkdf regression tests against the RFC 5869 Appendix A vectors.
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

check_fail() {
  local name=$1 needle=$2
  shift 2
  local out
  if out=$("$@" 2>&1); then
    echo "FAIL $name (command unexpectedly succeeded)"
    echo "  output: $out"
    fail=1
  elif [[ "$out" == *"$needle"* ]]; then
    echo "PASS $name"
  else
    echo "FAIL $name"
    echo "  expected substring: $needle"
    echo "  actual:             $out"
    fail=1
  fi
}

run() { uv run --script cryptography-tool -- "$@"; }

# --- RFC 5869 A.1: SHA-256, basic ---
IKM=0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b
SALT=000102030405060708090a0b0c
INFO=f0f1f2f3f4f5f6f7f8f9
PRK=077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5
OKM=3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865

check "hkdf derive (RFC 5869 A.1)" "$OKM" \
  "$(run hkdf derive --ikm "$IKM" --salt "$SALT" --info "$INFO" --length 42)"
check "hkdf extract (RFC 5869 A.1)" "$PRK" \
  "$(run hkdf extract --ikm "$IKM" --salt "$SALT")"
check "hkdf expand (RFC 5869 A.1)" "$OKM" \
  "$(run hkdf expand --prk "$PRK" --info "$INFO" --length 42)"

# --trace exposes the extract PRK and every full T(i) block.
TRACE=$(run hkdf derive --ikm "$IKM" --salt "$SALT" --info "$INFO" --length 42 --trace)
check "hkdf --trace PRK (RFC 5869 A.1)" "$PRK" \
  "$(sed -n 's/^prk     = \([0-9a-f]*\) .*/\1/p' <<<"$TRACE" | head -1)"
check "hkdf --trace T(2) full block" \
  "34007208d5b887185865b4b0a85a993b89b9b65683d60f0106d28fff039d0b6f" \
  "$(sed -n 's/^T(2)    = \([0-9a-f]*\).*/\1/p' <<<"$TRACE")"

# --- RFC 5869 A.2: SHA-256, long inputs/outputs ---
IKM2=000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f
SALT2=606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf
INFO2=b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff
PRK2=06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244
OKM2=b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c59045a99cac7827271cb41c65e590e09da3275600c2f09b8367793a9aca3db71cc30c58179ec3e87c14c01d5c1f3434f1d87

check "hkdf derive (RFC 5869 A.2)" "$OKM2" \
  "$(run hkdf derive --ikm "$IKM2" --salt "$SALT2" --info "$INFO2" --length 82)"
check "hkdf extract (RFC 5869 A.2)" "$PRK2" \
  "$(run hkdf extract --ikm "$IKM2" --salt "$SALT2")"
check "hkdf expand (RFC 5869 A.2)" "$OKM2" \
  "$(run hkdf expand --prk "$PRK2" --info "$INFO2" --length 82)"

# --- RFC 5869 A.3: SHA-256, zero-length (not omitted) salt/info ---
check "hkdf derive (RFC 5869 A.3, empty salt/info)" \
  "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8" \
  "$(run hkdf derive --ikm "$IKM" --salt "" --info "" --length 42)"
check "hkdf extract (RFC 5869 A.3, empty salt)" \
  "19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04" \
  "$(run hkdf extract --ikm "$IKM" --salt "")"

# --- RFC 5869 A.4-A.7: SHA-1 ---
check "hkdf derive (RFC 5869 A.4, SHA-1)" \
  "085a01ea1b10f36933068b56efa5ad81a4f14b822f5b091568a9cdd4f155fda2c22e422478d305f3f896" \
  "$(run hkdf derive --hash sha1 --ikm 0b0b0b0b0b0b0b0b0b0b0b --salt "$SALT" --info "$INFO" --length 42)"
check "hkdf extract (RFC 5869 A.4, SHA-1)" \
  "9b6c18c432a7bf8f0e71c8eb88f4b30baa2ba243" \
  "$(run hkdf extract --hash sha1 --ikm 0b0b0b0b0b0b0b0b0b0b0b --salt "$SALT")"
check "hkdf derive (RFC 5869 A.5, SHA-1 long)" \
  "0bd770a74d1160f7c9f12cd5912a06ebff6adcae899d92191fe4305673ba2ffe8fa3f1a4e5ad79f3f334b3b202b2173c486ea37ce3d397ed034c7f9dfeb15c5e927336d0441f4c4300e2cff0d0900b52d3b4" \
  "$(run hkdf derive --hash sha1 --ikm "$IKM2" --salt "$SALT2" --info "$INFO2" --length 82)"
check "hkdf extract (RFC 5869 A.6, SHA-1 empty salt)" \
  "da8c8a73c7fa77288ec6f5e7c297786aa0d32d01" \
  "$(run hkdf extract --hash sha1 --ikm "$IKM" --salt "")"
# A.7: salt not provided => HashLen zero octets.
check "hkdf derive (RFC 5869 A.7, SHA-1 default salt)" \
  "2c91117204d745f3500d636a62f64f0ab3bae548aa53d423b0d1f27ebba6f5e5673a081d70cce7acfc48" \
  "$(run hkdf derive --hash sha1 --ikm 0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c --length 42)"
check "hkdf default salt == explicit HashLen zeros" \
  "$(run hkdf extract --hash sha1 --ikm 0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c --salt 0000000000000000000000000000000000000000)" \
  "$(run hkdf extract --hash sha1 --ikm 0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c)"

# --- info separates derived keys; salt separates them too ---
check "hkdf info changes OKM" "different" \
  "$(if [[ "$(run hkdf derive --ikm "$IKM" --salt "$SALT" --length 32)" != \
           "$(run hkdf derive --ikm "$IKM" --salt "$SALT" --info 01 --length 32)" ]]; then
       echo different; else echo same; fi)"

# --- Limits: N = 255 blocks, and negative length ---
MAXLEN=$((255 * 32))
check "hkdf max length (255 * HashLen)" "$((MAXLEN * 2 + 1))" \
  "$(run hkdf derive --ikm "$IKM" --salt "$SALT" --length "$MAXLEN" | wc -c | tr -d ' ')"
check_fail "hkdf rejects length > 255 * HashLen" \
  "error: length must be <= 255 * HashLen = 8160 bytes" \
  run hkdf derive --ikm "$IKM" --length 8161
check_fail "hkdf rejects negative length" "error: length must be >= 0" \
  run hkdf derive --ikm "$IKM" --length -1

# --- Argument validation ---
check_fail "hkdf derive requires --ikm" "error: --ikm is required for derive" \
  run hkdf derive
check_fail "hkdf expand requires --prk" "error: --prk is required for expand" \
  run hkdf expand --ikm "$IKM"
check_fail "hkdf rejects --prk on derive" "error: --prk is only valid for expand" \
  run hkdf derive --ikm "$IKM" --prk "$PRK"
check_fail "hkdf expand rejects short PRK" \
  "error: PRK must be at least HashLen = 32 bytes for SHA-256" \
  run hkdf expand --prk 00112233 --length 16

exit "$fail"
