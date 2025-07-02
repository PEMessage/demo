
import matplotlib.pyplot as plt
import numpy as np


# See: https://en.wikipedia.org/w/index.php?title=Modified_frequency_modulation&oldid=1292086961
# See: GB-T 15120.2-2012 磁卡国标
# See: ISOIEC7811
def mfm_encode(data_bits, prev_bit=0):
    """
    Encode bits using MFM encoding
    Args:
        data_bits: List of bits to encode
        prev_bit: The last bit of previous data (default 0)
    Returns:
        List of encoded bits
    """
    encoded_bits = [prev_bit]

    for current_bit in data_bits:
        if current_bit == 1:
            if encoded_bits[-1] == 0:
                # '1' is always encoded as 01
                encoded_bits.extend([1, 0])
            else:
                encoded_bits.extend([0, 1])
        else:
            # '0' depends on previous bit
            if encoded_bits[-1] == 0:
                encoded_bits.extend([1, 1])  # 0 after 0 -> 11
            else:
                encoded_bits.extend([0, 0])  # 0 after 1 -> 00

        prev_bit = current_bit

    return encoded_bits[1:]  # remove prev_bit


def mfm_decode(encoded_bits):
    """
    Decode MFM-encoded bits back to original data
    Args:
        encoded_bits: List of MFM-encoded bits (must have even length)
    Returns:
        List of decoded bits
    """
    if len(encoded_bits) % 2 != 0:
        raise ValueError("Encoded bits must have even length")

    decoded_bits = []

    # Split into 2-element chunks
    for i in range(0, len(encoded_bits), 2):
        chunk = encoded_bits[i:i+2]

        if chunk == [1, 1] or chunk == [0, 0]:
            decoded_bits.append(0)
        else:
            decoded_bits.append(1)

    return decoded_bits


if __name__ == '__main__':
    examples = [
        [0, 0, 0, 0, 0],  # 0b00000 -> 1100110011
        [1, 1, 1, 1, 1],  # 0b11111 -> 1010101010
        [1, 0, 1, 0, 0],  # 0b10100 -> 1011010011
    ]

    for original_bits in examples:
        print(f"Original: {original_bits}")
        encoded = mfm_encode(original_bits)
        print(f"Encoded: {encoded}\n")
