#!/usr/bin/env python3
"""
DUKPT CLI Tool - Derive working keys from Initial Key (IK) and KSN.

Usage:
    python main.py --ksn <KSN> --ik <IK_HEX> --worktype <KEY_TYPE>

Example:
    python main.py --ksn 123456789012345600000001 --ik CE9CE0C101D1138F97FB6CAD4DF045A7 --worktype AES128
"""

import argparse
import sys

from dukpt import (
    KeyType,
    KeyUsage,
    derive_working_key_from_ik,
    parse_ksn,
    _get_ik_key_type
)


def parse_key_type(key_type_str: str) -> KeyType:
    """Parse key type string to KeyType enum."""
    key_type_map = {
        "2TDEA": KeyType._2TDEA,
        "3TDEA": KeyType._3TDEA,
        "AES128": KeyType.AES128,
        "AES192": KeyType.AES192,
        "AES256": KeyType.AES256,
    }
    key_type_upper = key_type_str.upper()
    if key_type_upper not in key_type_map:
        raise ValueError(f"Invalid key type: {key_type_str}. Valid types: {', '.join(key_type_map.keys())}")
    return key_type_map[key_type_upper]


def derive_all_work_keys(
    initial_key: bytes,
    derive_key_type: KeyType,
    working_key_type: KeyType,
    ksn: str,
) -> dict[str, bytes]:
    """Derive all work keys from IK and KSN."""
    keys = {}

    key_usages = [
        ("PIN_ENCRYPTION", KeyUsage.PIN_ENCRYPTION),
        ("MAC_GENERATION", KeyUsage.MAC_GENERATION),
        ("MAC_VERIFICATION", KeyUsage.MAC_VERIFICATION),
        ("MAC_BOTH", KeyUsage.MAC_BOTH),
        ("DATA_ENCRYPT", KeyUsage.DATA_ENCRYPT),
        ("DATA_DECRYPT", KeyUsage.DATA_DECRYPT),
        ("DATA_BOTH", KeyUsage.DATA_BOTH),
        ("KEY_ENCRYPTION", KeyUsage.KEY_ENCRYPTION),
        ("KEY_DERIVATION", KeyUsage.KEY_DERIVATION),
    ]

    for name, usage in key_usages:
        key = derive_working_key_from_ik(
            initial_key=initial_key,
            derive_key_type=derive_key_type,
            working_key_usage=usage,
            working_key_type=working_key_type,
            ksn=ksn,
        )
        keys[name] = key

    return keys


def main():
    parser = argparse.ArgumentParser(
        description="DUKPT CLI - Derive working keys from Initial Key (IK) and KSN",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python main.py --ksn 123456789012345600000001 --ik CE9CE0C101D1138F97FB6CAD4DF045A7 --worktype AES128
    python main.py --ksn 123456789012345600000001 --ik CE9CE0C101D1138F97FB6CAD4DF045A7 --worktype AES256 --derive-type AES256
        """,
    )

    parser.add_argument(
        "--ksn",
        required=True,
        help="Key Serial Number (24 hex characters)",
    )
    parser.add_argument(
        "--ik",
        required=True,
        help="Initial Key in hexadecimal",
    )
    parser.add_argument(
        "--worktype",
        required=True,
        choices=["2TDEA", "3TDEA", "AES128", "AES192", "AES256"],
        help="Working key type to derive",
    )
    # parser.add_argument(
    #     "--derive-type",
    #     # default="AES256",
    #     choices=["2TDEA", "3TDEA", "AES128", "AES192", "AES256"],
    #     help="Derive key type (usually matches IK type, default: AES256)",
    # )

    args = parser.parse_args()

    # Parse KSN
    try:
        initial_key_id, transaction_counter = parse_ksn(args.ksn)
    except ValueError as e:
        print(f"Error: Invalid KSN - {e}", file=sys.stderr)
        sys.exit(1)

    # Parse IK
    try:
        initial_key = bytes.fromhex(args.ik)
    except ValueError:
        print("Error: IK must be valid hexadecimal", file=sys.stderr)
        sys.exit(1)

    # Parse key types
    try:
        derive_key_type = _get_ik_key_type(args.ik)
        working_key_type = parse_key_type(args.worktype)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

    # Derive all work keys
    try:
        work_keys = derive_all_work_keys(
            initial_key=initial_key,
            derive_key_type=derive_key_type,
            working_key_type=working_key_type,
            ksn=args.ksn,
        )
    except Exception as e:
        print(f"Error deriving keys: {e}", file=sys.stderr)
        sys.exit(1)

    # Output results
    print(f"KSN: {args.ksn.upper()}")
    print(f"Initial Key ID: {initial_key_id.hex().upper()}")
    print(f"Transaction Counter: {transaction_counter:08X}")
    print(f"IK: {initial_key.hex().upper()}")
    print(f"Derive Key Type: {derive_key_type.value}")
    print(f"Working Key Type: {working_key_type.value}")
    print("")
    print("Derived Working Keys:")
    print("-" * 60)

    for name, key in work_keys.items():
        print(f"{name:<25} {key.hex().upper()}")


if __name__ == "__main__":
    main()
