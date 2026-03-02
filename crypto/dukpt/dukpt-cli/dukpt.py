"""
AES DUKPT (Derived Unique Key Per Transaction) - ANSI X9.24-3-2017

A minimal, pythonic implementation for deriving keys from BDK and KSN.

Example usage:
    # From BDK (full workflow)
    bdk = bytes.fromhex("FEDCBA9876543210F1F1F1F1F1F1F1F1")
    ksn = "123456789012345600000001"
    pin_key = derive_pin_encryption_key(
        bdk=bdk,
        derive_key_type=KeyType.AES128,
        working_key_type=KeyType.AES128,
        ksn=ksn,
    )

    # From Initial Key (when you already have IK)
    ik = bytes.fromhex("CE9CE0C101D1138F97FB6CAD4DF045A7...")
    pin_key = derive_pin_encryption_key_from_ik(
        initial_key=ik,
        derive_key_type=KeyType.AES256,
        working_key_type=KeyType.AES128,
        ksn=ksn,
    )
"""

from __future__ import annotations

from enum import Enum

from Crypto.Cipher import AES


class KeyType(Enum):
    """Key type definitions for DUKPT."""
    _2TDEA = "2TDEA"
    _3TDEA = "3TDEA"
    AES128 = "AES128"
    AES192 = "AES192"
    AES256 = "AES256"


class KeyUsage(Enum):
    """Key usage definitions for DUKPT."""
    KEY_ENCRYPTION = 0x0002
    PIN_ENCRYPTION = 0x1000
    MAC_GENERATION = 0x2000
    MAC_VERIFICATION = 0x2001
    MAC_BOTH = 0x2002
    DATA_ENCRYPT = 0x3000
    DATA_DECRYPT = 0x3001
    DATA_BOTH = 0x3002
    KEY_DERIVATION = 0x8000
    KEY_DERIVATION_INITIAL = 0x8001


def _key_length(key_type: KeyType) -> int:
    """Return key length in bits for a given key type."""
    lengths = {
        KeyType._2TDEA: 128,
        KeyType._3TDEA: 192,
        KeyType.AES128: 128,
        KeyType.AES192: 192,
        KeyType.AES256: 256,
    }
    return lengths[key_type]


def _algorithm_indicator(key_type: KeyType) -> bytes:
    """Return algorithm indicator bytes for derivation data."""
    indicators = {
        KeyType._2TDEA: bytes.fromhex("0000"),
        KeyType._3TDEA: bytes.fromhex("0001"),
        KeyType.AES128: bytes.fromhex("0002"),
        KeyType.AES192: bytes.fromhex("0003"),
        KeyType.AES256: bytes.fromhex("0004"),
    }
    return indicators[key_type]


def _length_bytes(key_type: KeyType) -> bytes:
    """Return length bytes for derivation data."""
    lengths = {
        KeyType._2TDEA: bytes.fromhex("0080"),
        KeyType._3TDEA: bytes.fromhex("00C0"),
        KeyType.AES128: bytes.fromhex("0080"),
        KeyType.AES192: bytes.fromhex("00C0"),
        KeyType.AES256: bytes.fromhex("0100"),
    }
    return lengths[key_type]


def _create_derivation_data(
    key_usage: KeyUsage,
    key_type: KeyType,
    initial_key_id: bytes,
    transaction_counter: int,
    is_initial_key: bool = False,
) -> list[bytes]:
    """
    Create derivation data blocks according to ANSI X9.24-3-2017.

    Returns a list of 16-byte blocks, one for each block of derived key material.
    """
    num_blocks = (_key_length(key_type) + 127) // 128
    blocks = []

    for block_counter in range(1, num_blocks + 1):
        data = bytearray(16)

        # Byte 0: Version
        data[0] = 0x01

        # Byte 1: Key Block Counter
        data[1] = block_counter

        # Bytes 2-3: Key Usage Indicator
        data[2:4] = key_usage.value.to_bytes(2, "big")

        # Bytes 4-5: Algorithm Indicator
        data[4:6] = _algorithm_indicator(key_type)

        # Bytes 6-7: Length
        data[6:8] = _length_bytes(key_type)

        # Bytes 8-15: depend on derivation purpose
        if is_initial_key:
            # For initial key: use full 64-bit Initial Key ID
            data[8:16] = initial_key_id
        else:
            # For working/derivation keys: use right 32 bits of IKID + 32-bit counter
            data[8:12] = initial_key_id[4:8]  # Right half of Initial Key ID (Derivation ID)
            data[12:16] = transaction_counter.to_bytes(4, "big")

        blocks.append(bytes(data))

    return blocks


def derive_key(derivation_key: bytes, key_type: KeyType, derivation_data: list[bytes]) -> bytes:
    """
    Derive a key using AES-ECB in counter mode (NIST SP800-108).

    Args:
        derivation_key: The key to derive from (AES-128, AES-192, or AES-256)
        key_type: The type of key to derive
        derivation_data: List of 16-byte derivation data blocks

    Returns:
        The derived key (length depends on key_type)
    """
    cipher = AES.new(derivation_key, AES.MODE_ECB)

    result = bytearray()
    for block in derivation_data:
        result.extend(cipher.encrypt(block))

    # Return only the required number of bytes for the key type
    key_len_bytes = _key_length(key_type) // 8
    return bytes(result[:key_len_bytes])


def derive_initial_key(bdk: bytes, key_type: KeyType, initial_key_id: bytes) -> bytes:
    """
    Derive the Initial DUKPT Key from the Base Derivation Key.

    Args:
        bdk: Base Derivation Key (AES-128, AES-192, or AES-256)
        key_type: Type of key being derived
        initial_key_id: 64-bit Initial Key ID (BDK ID || Derivation ID)

    Returns:
        The Initial DUKPT Key
    """
    derivation_data = _create_derivation_data(
        KeyUsage.KEY_DERIVATION_INITIAL,
        key_type,
        initial_key_id,
        0,
        is_initial_key=True,
    )
    return derive_key(bdk, key_type, derivation_data)


def _derive_working_key_from_initial_key(
    initial_key: bytes,
    derive_key_type: KeyType,
    working_key_usage: KeyUsage,
    working_key_type: KeyType,
    initial_key_id: bytes,
    transaction_counter: int,
) -> bytes:
    """
    Derive a working key from the Initial DUKPT Key for a given transaction.

    This is the core algorithm used after the Initial Key has been obtained
    (either derived from BDK or loaded directly).

    Args:
        initial_key: The Initial DUKPT Key (already derived from BDK)
        derive_key_type: Type of the derivation key (same as Initial Key type)
        working_key_usage: Usage of the working key (PIN, MAC, Data, etc.)
        working_key_type: Type of working key to derive
        initial_key_id: 64-bit Initial Key ID
        transaction_counter: 32-bit transaction counter

    Returns:
        The derived working key
    """
    # Step 1: Calculate current derivation key by processing bits in transaction counter
    mask = 0x80000000
    working_counter = 0
    derivation_key = initial_key

    while mask > 0:
        if transaction_counter & mask:
            working_counter |= mask
            derivation_data = _create_derivation_data(
                KeyUsage.KEY_DERIVATION,
                derive_key_type,
                initial_key_id,
                working_counter,
                is_initial_key=False,
            )
            derivation_key = derive_key(derivation_key, derive_key_type, derivation_data)
        mask >>= 1

    # Step 2: Derive the working key from the current derivation key
    derivation_data = _create_derivation_data(
        working_key_usage,
        working_key_type,
        initial_key_id,
        transaction_counter,
        is_initial_key=False,
    )
    working_key = derive_key(derivation_key, working_key_type, derivation_data)

    return working_key


def derive_working_key(
    bdk: bytes,
    derive_key_type: KeyType,
    working_key_usage: KeyUsage,
    working_key_type: KeyType,
    initial_key_id: bytes,
    transaction_counter: int,
) -> bytes:
    """
    Derive a working key from the Base Derivation Key for a given transaction.

    This follows the Host Security Module algorithm from the specification.

    Args:
        bdk: Base Derivation Key
        derive_key_type: Type of the derivation key (usually same as BDK type)
        working_key_usage: Usage of the working key (PIN, MAC, Data, etc.)
        working_key_type: Type of working key to derive
        initial_key_id: 64-bit Initial Key ID
        transaction_counter: 32-bit transaction counter

    Returns:
        The derived working key
    """
    # Step 1: Derive the Initial Key from BDK
    initial_key = derive_initial_key(bdk, derive_key_type, initial_key_id)

    # Step 2: Derive working key from Initial Key
    return _derive_working_key_from_initial_key(
        initial_key, derive_key_type, working_key_usage, working_key_type,
        initial_key_id, transaction_counter
    )


def parse_ksn(ksn: str | bytes) -> tuple[bytes, int]:
    """
    Parse a 96-bit KSN (Key Serial Number) into Initial Key ID and Transaction Counter.

    KSN format:
    - Bytes 0-7: Initial Key ID (64 bits) = BDK ID (32 bits) + Derivation ID (32 bits)
    - Bytes 8-11: Transaction Counter (32 bits)

    Args:
        ksn: 24 hex characters or 12 bytes

    Returns:
        Tuple of (initial_key_id: 8 bytes, transaction_counter: int)
    """
    if isinstance(ksn, str):
        ksn = bytes.fromhex(ksn)

    if len(ksn) != 12:
        raise ValueError(f"KSN must be 12 bytes (96 bits), got {len(ksn)}")

    initial_key_id = ksn[:8]
    transaction_counter = int.from_bytes(ksn[8:12], "big")

    return initial_key_id, transaction_counter


# =============================================================================
# Convenience functions for common key usages (from BDK)
# =============================================================================

def derive_pin_encryption_key(
    bdk: bytes,
    derive_key_type: KeyType,
    working_key_type: KeyType,
    ksn: str | bytes,
) -> bytes:
    """Derive a PIN encryption key from BDK."""
    initial_key_id, tc = parse_ksn(ksn)
    return derive_working_key(
        bdk, derive_key_type, KeyUsage.PIN_ENCRYPTION, working_key_type,
        initial_key_id, tc
    )


def derive_mac_key(
    bdk: bytes,
    derive_key_type: KeyType,
    working_key_type: KeyType,
    ksn: str | bytes,
    for_generation: bool = True,
) -> bytes:
    """Derive a MAC key (generation or verification) from BDK."""
    initial_key_id, tc = parse_ksn(ksn)
    usage = KeyUsage.MAC_GENERATION if for_generation else KeyUsage.MAC_VERIFICATION
    return derive_working_key(
        bdk, derive_key_type, usage, working_key_type, initial_key_id, tc
    )


def derive_data_encryption_key(
    bdk: bytes,
    derive_key_type: KeyType,
    working_key_type: KeyType,
    ksn: str | bytes,
    for_encryption: bool = True,
) -> bytes:
    """Derive a data encryption key (encrypt or decrypt) from BDK."""
    initial_key_id, tc = parse_ksn(ksn)
    usage = KeyUsage.DATA_ENCRYPT if for_encryption else KeyUsage.DATA_DECRYPT
    return derive_working_key(
        bdk, derive_key_type, usage, working_key_type, initial_key_id, tc
    )


def derive_key_encryption_key(
    bdk: bytes,
    derive_key_type: KeyType,
    working_key_type: KeyType,
    ksn: str | bytes,
) -> bytes:
    """Derive a key encryption key from BDK."""
    initial_key_id, tc = parse_ksn(ksn)
    return derive_working_key(
        bdk, derive_key_type, KeyUsage.KEY_ENCRYPTION, working_key_type,
        initial_key_id, tc
    )


# =============================================================================
# Convenience functions for common key usages (from Initial Key)
# =============================================================================

def derive_working_key_from_ik(
    initial_key: bytes,
    derive_key_type: KeyType,
    working_key_usage: KeyUsage,
    working_key_type: KeyType,
    ksn: str | bytes,
) -> bytes:
    """
    Derive a working key from the Initial DUKPT Key for a given KSN.

    This is useful when you already have the Initial Key (e.g., from test vectors
    or device initialization) rather than starting from the BDK.

    Args:
        initial_key: The Initial DUKPT Key
        derive_key_type: Type of the derivation key (same as Initial Key type)
        working_key_usage: Usage of the working key
        working_key_type: Type of working key to derive
        ksn: Key Serial Number (24 hex chars or 12 bytes)

    Returns:
        The derived working key
    """
    initial_key_id, tc = parse_ksn(ksn)
    return _derive_working_key_from_initial_key(
        initial_key, derive_key_type, working_key_usage, working_key_type,
        initial_key_id, tc
    )


def derive_pin_encryption_key_from_ik(
    initial_key: bytes,
    derive_key_type: KeyType,
    working_key_type: KeyType,
    ksn: str | bytes,
) -> bytes:
    """Derive a PIN encryption key from Initial Key."""
    return derive_working_key_from_ik(
        initial_key, derive_key_type, KeyUsage.PIN_ENCRYPTION, working_key_type, ksn
    )


def derive_mac_key_from_ik(
    initial_key: bytes,
    derive_key_type: KeyType,
    working_key_type: KeyType,
    ksn: str | bytes,
    for_generation: bool = True,
) -> bytes:
    """Derive a MAC key (generation or verification) from Initial Key."""
    usage = KeyUsage.MAC_GENERATION if for_generation else KeyUsage.MAC_VERIFICATION
    return derive_working_key_from_ik(
        initial_key, derive_key_type, usage, working_key_type, ksn
    )


def derive_data_encryption_key_from_ik(
    initial_key: bytes,
    derive_key_type: KeyType,
    working_key_type: KeyType,
    ksn: str | bytes,
    for_encryption: bool = True,
) -> bytes:
    """Derive a data encryption key (encrypt or decrypt) from Initial Key."""
    usage = KeyUsage.DATA_ENCRYPT if for_encryption else KeyUsage.DATA_DECRYPT
    return derive_working_key_from_ik(
        initial_key, derive_key_type, usage, working_key_type, ksn
    )


def derive_key_encryption_key_from_ik(
    initial_key: bytes,
    derive_key_type: KeyType,
    working_key_type: KeyType,
    ksn: str | bytes,
) -> bytes:
    """Derive a key encryption key from Initial Key."""
    return derive_working_key_from_ik(
        initial_key, derive_key_type, KeyUsage.KEY_ENCRYPTION, working_key_type, ksn
    )


# =============================================================================
# Test with ANSI X9.24-3-2017 test vectors
# =============================================================================

def _get_working_key_type(work_key_type_str: str) -> KeyType | None:
    """Convert string key type to KeyType enum."""
    type_map = {
        "AES-256": KeyType.AES256,
        "AES-192": KeyType.AES192,
        "AES-128": KeyType.AES128,
        "3TDEA": KeyType._3TDEA,
        "2TDEA": KeyType._2TDEA,
    }
    return type_map.get(work_key_type_str)


def _get_ik_key_type(ik_hex: str) -> KeyType:
    """Determine IK key type from its length."""
    ik_len = len(ik_hex) // 2
    if ik_len == 32:
        return KeyType.AES256
    elif ik_len == 24:
        return KeyType.AES192
    elif ik_len == 16:
        return KeyType.AES128
    else:
        raise ValueError(f"Unknown IK length: {ik_len} bytes")


def _run_tests(test_data: list, derive_key_type: KeyType | None = None) -> bool:
    """Run tests on a test data set."""
    all_passed = True

    for test in test_data:
        work_key_type = test["WorK_Key_Type"]
        ik_hex = test["IK"]
        ksn = test["KSN"]

        working_key_type = _get_working_key_type(work_key_type)
        if working_key_type is None:
            continue

        # Parse KSN
        initial_key_id, tc = parse_ksn(ksn)

        # The IK in test data is already the Initial DUKPT Key
        ik = bytes.fromhex(ik_hex)

        # Determine derive_key_type from IK length if not specified
        if derive_key_type is None:
            test_derive_key_type = _get_ik_key_type(ik_hex)
        else:
            test_derive_key_type = derive_key_type

        print(f"\n=== Test: {work_key_type} (IK: {test_derive_key_type.value}) ===")
        print(f"KSN: {ksn}")
        print(f"Initial Key ID: {initial_key_id.hex().upper()}")
        print(f"Transaction Counter: {tc:08X}")

        # Test all key types from the test data
        test_keys = [
            ("PIN_Encryption_Key", KeyUsage.PIN_ENCRYPTION),
            ("Msg_Auth_Gen_Key", KeyUsage.MAC_GENERATION),
            ("Msg_Auth_Ver_Key", KeyUsage.MAC_VERIFICATION),
            ("M_Auth_Both_Ways_Key", KeyUsage.MAC_BOTH),
            ("Data_Encr_Encrypt_Key", KeyUsage.DATA_ENCRYPT),
            ("Data_Encr_Decrypt_Key", KeyUsage.DATA_DECRYPT),
            ("D_Encr_Both_Ways_Key", KeyUsage.DATA_BOTH),
            ("Key_Encryption_Key", KeyUsage.KEY_ENCRYPTION),
            ("Key_Derivation_Key", KeyUsage.KEY_DERIVATION),
        ]

        for test_key_name, key_usage in test_keys:
            derived_key = _derive_working_key_from_initial_key(
                ik, test_derive_key_type, key_usage, working_key_type,
                initial_key_id, tc
            )
            expected_key = bytes.fromhex(test[test_key_name])

            match = derived_key == expected_key
            if not match:
                all_passed = False

            status = "PASS" if match else "FAIL"
            print(f"  {test_key_name}: {status}")
            if not match:
                print(f"    Expected: {expected_key.hex().upper()}")
                print(f"    Got:      {derived_key.hex().upper()}")

    return all_passed


if __name__ == "__main__":
    import json

    all_passed = True

    # Test 1: Original test data (AES-256 IK)
    print("\n" + "=" * 50)
    print("TEST SET 1: AES-256 Initial Key")
    print("=" * 50)

    with open("test/data.json") as f:
        test_data = json.load(f)

    if not _run_tests(test_data, KeyType.AES256):
        all_passed = False

    # Test 2: AES-128 IK test data
    print("\n" + "=" * 50)
    print("TEST SET 2: AES-128 Initial Key")
    print("=" * 50)

    with open("test/data_ik_aes128.json") as f:
        test_data_ik128 = json.load(f)

    if not _run_tests(test_data_ik128, KeyType.AES128):
        all_passed = False

    # Final summary
    print(f"\n{'='*50}")
    if all_passed:
        print("All tests passed!")
    else:
        print("Some tests failed!")
