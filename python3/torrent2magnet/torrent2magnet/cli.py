import argparse
import sys
from pathlib import Path
from .core import torrent_to_magnet

def main():
    parser = argparse.ArgumentParser(
        description="Convert torrent file to magnet link",
        prog="torrent2magnet"
    )
    parser.add_argument(
        "torrent_file",
        help="Path to torrent file"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Show additional information"
    )

    args = parser.parse_args()

    torrent_path = Path(args.torrent_file)

    if not torrent_path.exists():
        print(f"Error: File '{torrent_path}' not found", file=sys.stderr)
        sys.exit(1)

    if not torrent_path.is_file():
        print(f"Error: '{torrent_path}' is not a file", file=sys.stderr)
        sys.exit(1)

    try:
        magnet_link = torrent_to_magnet(torrent_path)
        print(magnet_link)

        if args.verbose:
            print(f"\nConverted: {torrent_path} -> magnet link", file=sys.stderr)

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()

