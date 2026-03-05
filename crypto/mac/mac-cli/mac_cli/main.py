"""Minimal entry point for the MAC CLI tool."""

import argparse
import sys

from mac_cli import calculate_mac, MACAlgorithm, PaddingMethod


def main():
    parser = argparse.ArgumentParser(
        description="ISO/IEC 9797-1 MAC (Message Authentication Code) calculator"
    )
    parser.add_argument(
        "-a", "--algorithm",
        choices=["alg1", "alg2", "alg3", "alg5"],
        default="alg5",
        help="MAC algorithm (default: alg5/CMAC)"
    )
    parser.add_argument(
        "-c", "--cipher",
        choices=["AES", "DES", "3DES"],
        default="AES",
        help="Block cipher (default: AES)"
    )
    parser.add_argument(
        "-k", "--key",
        required=True,
        help="Hex-encoded key"
    )
    parser.add_argument(
        "-d", "--data",
        required=True,
        help="Hex-encoded data to authenticate"
    )
    parser.add_argument(
        "-m", "--mac-bits",
        type=int,
        default=None,
        help="MAC length in bits (default: full block size)"
    )
    parser.add_argument(
        "-p", "--padding",
        choices=["method1", "method2", "method3"],
        default="method2",
        help="Padding method (default: method2)"
    )

    args = parser.parse_args()

    # Parse key and data from hex
    try:
        key = bytes.fromhex(args.key)
        data = bytes.fromhex(args.data)
    except ValueError as e:
        print(f"Error: Invalid hex string - {e}", file=sys.stderr)
        sys.exit(1)

    # Map algorithm string to enum
    algo_map = {
        "alg1": MACAlgorithm.ALG_1,
        "alg2": MACAlgorithm.ALG_2,
        "alg3": MACAlgorithm.ALG_3,
        "alg5": MACAlgorithm.ALG_5,
    }
    algorithm = algo_map[args.algorithm]

    # Map padding string to enum
    padding_map = {
        "method1": PaddingMethod.METHOD_1,
        "method2": PaddingMethod.METHOD_2,
        "method3": PaddingMethod.METHOD_3,
    }
    padding = padding_map[args.padding]

    try:
        mac = calculate_mac(
            data=data,
            key=key,
            algorithm=algorithm,
            cipher_name=args.cipher,
            mac_bits=args.mac_bits,
            padding_method=padding,
        )
        print(mac.hex())
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
