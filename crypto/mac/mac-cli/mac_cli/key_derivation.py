"""Key derivation methods for ISO/IEC 9797-1 MAC algorithms."""

from enum import Enum, auto
from typing import Callable

from Crypto.Cipher import AES


def _encrypt_block(cipher_func: Callable[[bytes], bytes], block: bytes) -> bytes:
    """Encrypt a single block."""
    return cipher_func(block)


def derive_keys_kdm1(
    key: bytes, block_size: int, encrypt_func: Callable[[bytes], bytes]
) -> tuple[bytes, bytes]:
    """Key Derivation Method 1 - derives two keys K' and K'' from K.

    Uses Counter Method (CTR) as defined in ISO/IEC 10116.

    Args:
        key: Master key K
        block_size: Block size in bits (n)
        encrypt_func: Function to encrypt a block

    Returns:
        Tuple of (K', K'')
    """
    n = block_size // 8
    k = len(key) * 8
    t = (k + block_size - 1) // block_size  # Smallest integer >= k/n

    # Generate S1: encrypt CT_1 through CT_t
    s1 = b""
    for i in range(1, t + 1):
        ct_i = i.to_bytes(n, "big")
        s1 += encrypt_func(ct_i)

    # Generate S2: encrypt CT_{t+1} through CT_{2t}
    s2 = b""
    for i in range(t + 1, 2 * t + 1):
        ct_i = i.to_bytes(n, "big")
        s2 += encrypt_func(ct_i)

    # K' = first k bits of S1
    k_prime = s1[: len(key)]
    # K'' = first k bits of S2
    k_double_prime = s2[: len(key)]

    return k_prime, k_double_prime


def multx(t: bytes, block_size: int) -> bytes:
    """Multiply by x in GF(2^n) finite field.

    This operation is defined as:
    - If leftmost bit of T is 0: T << 1
    - If leftmost bit of T is 1: (T << 1) XOR P_n

    Args:
        t: Input n-bit string
        block_size: Block size in bits (n)

    Returns:
        Result of multx(T)
    """
    n = block_size

    # Convert to integer for bit manipulation
    t_int = int.from_bytes(t, "big")

    # Left shift by 1
    result = (t_int << 1) & ((1 << n) - 1)

    # If leftmost bit was 1, XOR with P_n
    # P_n for n=128: x^128 + x^7 + x^2 + x + 1 -> 0^120 || 10000111
    # P_n for n=64: x^64 + x^4 + x^3 + x + 1 -> 0^59 || 11011
    if (t_int >> (n - 1)) & 1:
        if n == 128:
            p_n = 0x87  # 0^120 || 10000111
        elif n == 64:
            p_n = 0x1B  # 0^59 || 11011
        else:
            raise ValueError(f"Unsupported block size: {n}")
        result ^= p_n

    return result.to_bytes(n // 8, "big")


def derive_keys_kdm2(
    key: bytes, block_size: int, encrypt_func: Callable[[bytes], bytes]
) -> tuple[bytes, bytes]:
    """Key Derivation Method 2 - derives two masking keys K1 and K2 from K.

    This is used by MAC Algorithm 5 (CMAC/OMAC1).

    Args:
        key: Master key K
        block_size: Block size in bits (n)
        encrypt_func: Function to encrypt a block

    Returns:
        Tuple of (K1, K2) masking keys
    """
    n = block_size // 8

    # S = e_K(0^n)
    zero_block = b"\x00" * n
    s = encrypt_func(zero_block)

    # K1 = multx(S)
    k1 = multx(s, block_size)

    # K2 = multx(K1)
    k2 = multx(k1, block_size)

    return k1, k2
