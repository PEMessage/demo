"""Padding methods for ISO/IEC 9797-1 MAC algorithms."""

from enum import Enum, auto


class PaddingMethod(Enum):
    """ISO/IEC 9797-1 padding methods."""

    METHOD_1 = auto()  # Right-pad with zeros
    METHOD_2 = auto()  # Right-pad with 0x80 followed by zeros
    METHOD_3 = auto()  # Length-prepended, then right-pad with zeros
    METHOD_4 = auto()  # Conditional padding with 0x80


def pad(data: bytes, block_size: int, method: PaddingMethod) -> bytes:
    """Apply padding to data according to ISO/IEC 9797-1.

    Args:
        data: Input data to pad
        block_size: Block size in bits (n)
        method: Padding method to use

    Returns:
        Padded data
    """
    n = block_size // 8  # Convert bits to bytes
    bit_len = len(data) * 8

    match method:
        case PaddingMethod.METHOD_1:
            # Right-pad with '0' bits to multiple of n bits
            # Note: If data is empty, we still need to pad to one block
            if len(data) == 0:
                return b"\x00" * n
            if len(data) % n == 0:
                return data
            pad_len = n - (len(data) % n)
            return data + b"\x00" * pad_len

        case PaddingMethod.METHOD_2:
            # Right-pad with single '1' bit (0x80), then '0' bits
            data = data + b"\x80"
            if len(data) % n != 0:
                pad_len = n - (len(data) % n)
                data = data + b"\x00" * pad_len
            return data

        case PaddingMethod.METHOD_3:
            # Left-pad with length block, then right-pad with zeros
            # Length block: binary representation of L_D, left-padded to n bits
            length_block = bit_len.to_bytes(n, "big")
            data = data + b"\x00" * ((n - (len(data) % n)) % n)
            return length_block + data

        case PaddingMethod.METHOD_4:
            # Conditional padding: only if not already multiple of n
            if len(data) % n == 0:
                return data
            data = data + b"\x80"
            if len(data) % n != 0:
                pad_len = n - (len(data) % n)
                data = data + b"\x00" * pad_len
            return data

        case _:
            raise ValueError(f"Unknown padding method: {method}")


def unpad(data: bytes, block_size: int, method: PaddingMethod) -> bytes:
    """Remove padding from data (for verification purposes).

    Args:
        data: Padded data
        block_size: Block size in bits
        method: Padding method used

    Returns:
        Unpadded data
    """
    n = block_size // 8

    match method:
        case PaddingMethod.METHOD_1:
            return data.rstrip(b"\x00")

        case PaddingMethod.METHOD_2 | PaddingMethod.METHOD_4:
            # Remove trailing zeros and the 0x80 byte
            while data and data[-1] == 0:
                data = data[:-1]
            if data and data[-1] == 0x80:
                data = data[:-1]
            return data

        case PaddingMethod.METHOD_3:
            # Remove length block and trailing zeros
            if len(data) < n:
                return b""
            data = data[n:]  # Remove length block
            return data.rstrip(b"\x00")

        case _:
            raise ValueError(f"Unknown padding method: {method}")
