"""ISO/IEC 9797-1 MAC (Message Authentication Code) algorithms."""

from enum import Enum, auto
from typing import Callable

from Crypto.Cipher import AES
from Crypto.Cipher import DES3

from .padding import PaddingMethod, pad
from .key_derivation import derive_keys_kdm1, derive_keys_kdm2, multx


class KeyDerivationMethod(Enum):
    """Key derivation methods for MAC algorithms."""

    METHOD_1 = auto()  # Derives K' and K'' from K using CTR
    METHOD_2 = auto()  # Derives K1 and K2 from K using multx


class MACAlgorithm(Enum):
    """ISO/IEC 9797-1 MAC algorithms."""

    ALG_1 = auto()  # CBC-MAC (basic)
    ALG_2 = auto()  # EMAC - encrypt last block with different key
    ALG_3 = auto()  # Retail MAC - decrypt with K' then encrypt with K
    ALG_4 = auto()  # MacDES - double encrypt first block
    ALG_5 = auto()  # CMAC/OMAC1 - uses K1/K2 masking keys
    ALG_6 = auto()  # LMAC - encrypt last block with different key


def _xor_bytes(a: bytes, b: bytes) -> bytes:
    """XOR two byte strings."""
    return bytes(x ^ y for x, y in zip(a, b))


def _get_cipher(cipher_name: str, key: bytes):
    """Get cipher instance based on name."""
    if cipher_name.upper() in ("AES",):
        return AES.new(key, AES.MODE_ECB)
    elif cipher_name.upper() in ("DES3", "3DES", "TRIPLEDES"):
        return DES3.new(key, DES3.MODE_ECB)
    elif cipher_name.upper() in ("DES", "DEA"):
        from Crypto.Cipher import DES

        return DES.new(key, DES.MODE_ECB)
    else:
        raise ValueError(f"Unknown cipher: {cipher_name}")


def _cbc_iteration(
    blocks: list[bytes],
    key: bytes,
    cipher_name: str,
    initial: bytes | None = None,
) -> bytes:
    """Perform CBC iteration on blocks.

    Args:
        blocks: List of n-bit blocks
        key: Block cipher key
        cipher_name: Name of cipher to use
        initial: Initial value (H_0), defaults to zeros

    Returns:
        H_q (result after processing all blocks)
    """
    cipher = _get_cipher(cipher_name, key)
    n = cipher.block_size

    if initial is None:
        h = b"\x00" * n
    else:
        h = initial

    for block in blocks:
        h = cipher.encrypt(_xor_bytes(block, h))

    return h


def mac_algorithm_1(
    data: bytes,
    key: bytes,
    cipher_name: str,
    mac_bits: int,
    padding_method: PaddingMethod,
) -> bytes:
    """MAC Algorithm 1 - Basic CBC-MAC.

    Uses Final Iteration 1 and Output Transformation 1.
    Simple CBC-MAC with single key.

    Args:
        data: Input data
        key: Block cipher key K
        cipher_name: Cipher to use (e.g., 'AES', 'DES')
        mac_bits: Length of MAC in bits (m)
        padding_method: Padding method to use

    Returns:
        MAC of m bits
    """
    cipher = _get_cipher(cipher_name, key)
    n = cipher.block_size * 8  # bits

    # Step 1: Padding
    padded = pad(data, n, padding_method)

    # Step 2: Split into blocks
    blocks = [padded[i : i + cipher.block_size] for i in range(0, len(padded), cipher.block_size)]

    # Step 3: CBC iteration (all blocks)
    h = _cbc_iteration(blocks, key, cipher_name)

    # Step 4: Truncation (Output Transformation 1 is identity)
    mac_bytes = mac_bits // 8
    return h[:mac_bytes]


def mac_algorithm_2(
    data: bytes,
    key: bytes,
    key_prime: bytes,
    cipher_name: str,
    mac_bits: int,
    padding_method: PaddingMethod,
) -> bytes:
    """MAC Algorithm 2 - EMAC.

    Uses Final Iteration 1 and Output Transformation 2.
    Encrypts last block with K', then encrypts result with K' again.

    Args:
        data: Input data
        key: Block cipher key K for iteration
        key_prime: Block cipher key K' for output transformation
        cipher_name: Cipher to use
        mac_bits: Length of MAC in bits (m)
        padding_method: Padding method to use

    Returns:
        MAC of m bits
    """
    cipher = _get_cipher(cipher_name, key)
    n = cipher.block_size * 8

    # Padding
    padded = pad(data, n, padding_method)

    # Split into blocks
    blocks = [padded[i : i + cipher.block_size] for i in range(0, len(padded), cipher.block_size)]

    # CBC iteration with key K
    h = _cbc_iteration(blocks, key, cipher_name)

    # Output Transformation 2: encrypt with K'
    cipher_prime = _get_cipher(cipher_name, key_prime)
    g = cipher_prime.encrypt(h)

    # Truncation
    mac_bytes = mac_bits // 8
    return g[:mac_bytes]


def mac_algorithm_3(
    data: bytes,
    key: bytes,
    key_prime: bytes,
    cipher_name: str,
    mac_bits: int,
    padding_method: PaddingMethod,
) -> bytes:
    """MAC Algorithm 3 - ANSI Retail MAC.

    Uses Final Iteration 1 and Output Transformation 3.
    Decrypts with K', then encrypts with K.

    Args:
        data: Input data
        key: Block cipher key K
        key_prime: Block cipher key K'
        cipher_name: Cipher to use
        mac_bits: Length of MAC in bits (m)
        padding_method: Padding method to use

    Returns:
        MAC of m bits
    """
    cipher = _get_cipher(cipher_name, key)
    n = cipher.block_size * 8

    # Padding
    padded = pad(data, n, padding_method)

    # Split into blocks
    blocks = [padded[i : i + cipher.block_size] for i in range(0, len(padded), cipher.block_size)]

    # CBC iteration with key K
    h = _cbc_iteration(blocks, key, cipher_name)

    # Output Transformation 3: decrypt with K', then encrypt with K
    cipher_prime = _get_cipher(cipher_name, key_prime)
    decrypted = cipher_prime.decrypt(h)
    g = cipher.encrypt(decrypted)

    # Truncation
    mac_bytes = mac_bits // 8
    return g[:mac_bytes]


def mac_algorithm_5(
    data: bytes,
    key: bytes,
    cipher_name: str,
    mac_bits: int,
) -> bytes:
    """MAC Algorithm 5 - CMAC/OMAC1.

    Uses Key Derivation Method 2, Final Iteration 3, Output Transformation 1.
    This is the recommended modern MAC algorithm.

    Args:
        data: Input data
        key: Block cipher key K
        cipher_name: Cipher to use
        mac_bits: Length of MAC in bits (m)

    Returns:
        MAC of m bits
    """
    cipher = _get_cipher(cipher_name, key)
    n = cipher.block_size * 8
    block_size = cipher.block_size

    # Derive masking keys K1 and K2 using KDM2
    def encrypt_func(block: bytes) -> bytes:
        return cipher.encrypt(block)

    k1, k2 = derive_keys_kdm2(key, n, encrypt_func)

    # Padding Method 4: conditional padding
    if len(data) % block_size == 0 and len(data) > 0:
        # No padding needed, use K1
        padded = data
        use_k1 = True
    else:
        # Pad with 0x80 followed by zeros
        padded = data + b"\x80"
        if len(padded) % block_size != 0:
            padded += b"\x00" * (block_size - (len(padded) % block_size))
        use_k1 = False

    # Split into blocks
    blocks = [padded[i : i + block_size] for i in range(0, len(padded), block_size)]

    if not blocks:
        # Empty message case
        blocks = [b"\x00" * block_size]
        use_k1 = False

    # Process all blocks except last with CBC
    if len(blocks) > 1:
        h = _cbc_iteration(blocks[:-1], key, cipher_name)
    else:
        h = b"\x00" * block_size

    # Final Iteration 3: XOR with K1 or K2, then encrypt
    last_block = blocks[-1]
    if use_k1:
        last_block = _xor_bytes(last_block, k1)
    else:
        last_block = _xor_bytes(last_block, k2)

    h = _xor_bytes(last_block, h)
    h = cipher.encrypt(h)

    # Output Transformation 1 is identity
    # Truncation
    mac_bytes = mac_bits // 8
    return h[:mac_bytes]


def calculate_mac(
    data: bytes,
    key: bytes,
    algorithm: MACAlgorithm,
    cipher_name: str = "AES",
    mac_bits: int | None = None,
    padding_method: PaddingMethod | None = None,
    key_derivation: KeyDerivationMethod | None = None,
    key_prime: bytes | None = None,
) -> bytes:
    """Calculate MAC using specified ISO/IEC 9797-1 algorithm.

    Args:
        data: Input data to authenticate
        key: Primary key K
        algorithm: MAC algorithm to use
        cipher_name: Block cipher ('AES', 'DES', '3DES')
        mac_bits: Length of MAC in bits (default: full block size)
        padding_method: Padding method (required for ALG_1, ALG_2, ALG_3, ALG_4, ALG_6)
        key_derivation: Key derivation method (used by ALG_2, ALG_4, ALG_6)
        key_prime: Secondary key K' (if not derived)

    Returns:
        MAC value
    """
    # Determine block size from cipher
    if cipher_name.upper() == "AES":
        block_bits = 128
    elif cipher_name.upper() in ("DES", "DEA"):
        block_bits = 64
    elif cipher_name.upper() in ("DES3", "3DES", "TRIPLEDES"):
        block_bits = 64
    else:
        raise ValueError(f"Unknown cipher: {cipher_name}")

    # Default MAC length is full block size
    if mac_bits is None:
        mac_bits = block_bits

    match algorithm:
        case MACAlgorithm.ALG_1:
            if padding_method is None:
                padding_method = PaddingMethod.METHOD_2
            return mac_algorithm_1(data, key, cipher_name, mac_bits, padding_method)

        case MACAlgorithm.ALG_2:
            if padding_method is None:
                padding_method = PaddingMethod.METHOD_2
            if key_prime is None and key_derivation == KeyDerivationMethod.METHOD_1:
                # Derive K' from K
                cipher = _get_cipher(cipher_name, key)
                key_prime, _ = derive_keys_kdm1(key, block_bits, cipher.encrypt)
            if key_prime is None:
                raise ValueError("MAC Algorithm 2 requires key_prime or key_derivation")
            return mac_algorithm_2(data, key, key_prime, cipher_name, mac_bits, padding_method)

        case MACAlgorithm.ALG_3:
            if padding_method is None:
                padding_method = PaddingMethod.METHOD_2
            if key_prime is None:
                raise ValueError("MAC Algorithm 3 requires key_prime")
            return mac_algorithm_3(data, key, key_prime, cipher_name, mac_bits, padding_method)

        case MACAlgorithm.ALG_5:
            # ALG_5 uses Padding Method 4 internally
            return mac_algorithm_5(data, key, cipher_name, mac_bits)

        case _:
            raise ValueError(f"Algorithm {algorithm} not yet implemented")
