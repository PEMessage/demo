# AGENTS.md — cryptography-tool Development Notes

`cryptography-tool` provides a CLI over the [`cryptography`](https://cryptography.io)
library (pyca/cryptography). Main object handled: the `cryptography` Python package.

## Best Learning How to Use cryptography

The best way to learn `cryptography` is from its **official docs
(https://cryptography.io) plus live inspection**. Unlike pure-Python libraries,
many primitives are implemented in Rust (`cryptography.hazmat.bindings._rust`),
so docstrings are sparse — verify signatures at runtime instead of reading
source. Locate the installed package first:

```bash
uv run --with cryptography python3 -c "import cryptography ; print(cryptography.__version__, cryptography.__path__)"
```

Key areas of the API surface:

- `cryptography.hazmat.primitives.ciphers` — `Cipher` + `algorithms` (AES, SM4,
  ChaCha20, TripleDES…) + `modes` (ECB, CBC, CTR, GCM…)
- `cryptography.hazmat.primitives.ciphers.aead` — one-shot AEAD classes
  (`AESGCM`, `AESCCM`, `AESSIV`, `ChaCha20Poly1305`…)
- `cryptography.hazmat.primitives.hashes` / `hmac` / `cmac` — digests & MACs
- `cryptography.hazmat.primitives.asymmetric` — `rsa`, `ec`, `ed25519`, `x25519`, `padding`
- `cryptography.hazmat.primitives.kdf` — `pbkdf2`, `hkdf`, `scrypt`…
- `cryptography.hazmat.primitives.serialization` — PEM/DER key load & dump
- `cryptography.hazmat.primitives.padding` — PKCS7 / ANSI X9.23 block padding
- `cryptography.x509` — certificates, CSRs, CRLs
- `cryptography.fernet` — high-level recipe layer

## Toolchain: `uv run --script`

The CLI entry point is a **PEP 723 inline-script** (the `/// script` header
block). uv reads this metadata, creates an ephemeral venv, and auto-installs
dependencies — no `pyproject.toml` or manual `pip install` needed.

```bash
# Add a dependency (writes into the /// script header block)
uv add --script cryptography-tool cryptography

# Run (uv manages venv + deps automatically)
uv run --script cryptography-tool -- <subcommand> ...

# Experiment in isolation (does not touch project files)
uv run --with cryptography python -c "..."
```

## How to Explore cryptography's APIs

### Rule: Always Verify in Isolation First

When you need to understand a `cryptography` class (does the key need to be
`bytes`? which modes require an IV? what does `finalize()` return?), **use
`uv run --with cryptography python -c "..."` to build a minimal reproduction
in memory**. Confirm the behaviour before touching the CLI.

This avoids the cycle of: edit the main file → run → error → edit again.

### Snippet: Prove a Primitive End-to-End

Build the smallest possible encrypt/decrypt round-trip to confirm the contract
before wiring it to a CLI argument:

```bash
uv run --with cryptography python -c "
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
key = bytes.fromhex('000102030405060708090a0b0c0d0e0f')
enc = Cipher(algorithms.AES(key), modes.ECB()).encryptor()
ct = enc.update(bytes(16)) + enc.finalize()
print(ct.hex())
"
```

### Snippet: Inspect a Module's Public Surface

Dump the callables with signatures to discover the API instead of guessing
(`__all__` is not always defined — fall back to `dir()`):

```bash
uv run --with cryptography python -c "
import cryptography.hazmat.primitives.ciphers.aead as m, inspect
for name in dir(m):
    if name.startswith('_'): continue
    obj = getattr(m, name)
    if inspect.isclass(obj):
        print(name, [x for x in dir(obj) if not x.startswith('_')])
"
```

### Snippet: Confirm the Type / Error Contract

`cryptography` is strict: keys/data/IVs are **raw `bytes`** (via
`bytes.fromhex(...)`), never hex `str`. Bad parameters raise `ValueError`;
failed authentication raises `cryptography.exceptions.InvalidTag` /
`InvalidSignature` — trigger them to learn the boundary:

```bash
uv run --with cryptography python -c "
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.exceptions import InvalidTag
try:
    AESGCM(b'123')
except ValueError as e:
    print('caught:', e)
try:
    AESGCM(bytes(16)).decrypt(bytes(12), bytes(16), None)
except InvalidTag:
    print('caught: InvalidTag')
"
```

### Snippet: Cross-Check Against the Official Docs

For anything subtle (nonce sizes, allowed key lengths, deprecations), the
authoritative reference is https://cryptography.io/en/latest/ — match the
installed version printed above to the docs version.

## Workflow Summary

1. **`uv run --with cryptography python -c "..."`** — experiment with the API in isolation
2. **Check the official docs + runtime signatures** — parameter rules per primitive
3. **After confirming behaviour** — wire it into the CLI subcommand
4. **`./test.sh`** — run the full regression suite

## File Layout

```
cryptography-tool  # main entry point (PEP 723 inline-script, provides the CLI)
test/              # per-module test-data generators / fixtures
test.sh            # quick regression suite
```
