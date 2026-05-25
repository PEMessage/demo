# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "pycryptodome>=3.23.0",
# ]
# ///

import argparse
import sys
from pathlib import Path

from Crypto.Cipher import DES3

# > openssl rand -hex 24
# 1c2f7849077df96c662ba0af927be4365fccf73a9e334a5e
TDES_KEY_HEX = "1C2F7849077DF96C662BA0AF927BE4365FCCF73A9E334A5E"
TDES_IV_HEX = "0000000000000000"
BLOCK_SIZE = 8


def _get_cipher(key_hex: str) -> DES3:
    key = bytes.fromhex(key_hex)
    iv = bytes.fromhex(TDES_IV_HEX)
    return DES3.new(key, DES3.MODE_CBC, iv=iv)


# DER encoding does not make sure you dont have 00 ending
# padding and unpad zero might break content
# https://chat.deepseek.com/share/olzwjkroqguaqib3c2
def _zero_pad(data: bytes) -> bytes:
    pad_len = BLOCK_SIZE - (len(data) % BLOCK_SIZE)
    if pad_len == BLOCK_SIZE:
        pad_len = 0
    return data + b"\x00" * pad_len


def _zero_unpad(data: bytes) -> bytes:
    return data.rstrip(b"\x00")


def encrypt(in_path: Path, out_path: Path, key_hex: str) -> None:
    plaintext = in_path.read_bytes()
    padded = _zero_pad(plaintext)
    cipher = _get_cipher(key_hex)
    ciphertext = cipher.encrypt(padded)
    out_path.write_bytes(ciphertext)
    print(f"Encrypted {in_path} -> {out_path} ({len(plaintext)} -> {len(ciphertext)} bytes)")


def decrypt(in_path: Path, out_path: Path, key_hex: str) -> None:
    ciphertext = in_path.read_bytes()
    cipher = _get_cipher(key_hex)
    padded = cipher.decrypt(ciphertext)
    plaintext = _zero_unpad(padded)
    out_path.write_bytes(plaintext)
    print(f"Decrypted {in_path} -> {out_path} ({len(ciphertext)} -> {len(plaintext)} bytes)")


def main() -> None:
    parser = argparse.ArgumentParser(description="3DES-CBC encrypt/decrypt tool (zero IV, zero padding)")
    subparsers = parser.add_subparsers(dest="command", required=True)

    key_kwargs = dict(
        default=TDES_KEY_HEX,
        help=f"TDES key in hex (default: {TDES_KEY_HEX})",
    )

    enc_parser = subparsers.add_parser("encrypt", help="Encrypt a file")
    enc_parser.add_argument("-i", "--input", type=Path, required=True, help="Input file to encrypt")
    enc_parser.add_argument("-o", "--output", type=Path, required=True, help="Output encrypted file")
    enc_parser.add_argument("-k", "--key", **key_kwargs)

    dec_parser = subparsers.add_parser("decrypt", help="Decrypt a file")
    dec_parser.add_argument("-i", "--input", type=Path, required=True, help="Input file to decrypt")
    dec_parser.add_argument("-o", "--output", type=Path, required=True, help="Output decrypted file")
    dec_parser.add_argument("-k", "--key", **key_kwargs)

    args = parser.parse_args()

    if args.command == "encrypt":
        encrypt(args.input, args.output, args.key)
    elif args.command == "decrypt":
        decrypt(args.input, args.output, args.key)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
