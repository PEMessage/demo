

def count_one_bits(n: int) -> int:
    """Count the number of '1' bits in n."""
    return bin(n).count('1')

def least_significant_bit(n: int) -> int:
    """Get the value of the least significant set bit."""
    return n & -n
