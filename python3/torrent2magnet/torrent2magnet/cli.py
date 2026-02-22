import argparse
import sys
from pathlib import Path
from .core import torrent_to_magnet

def main():
    parser = argparse.ArgumentParser(
        description="Convert torrent file(s) to magnet link(s)",
        prog="torrent2magnet"
    )
    parser.add_argument(
        "torrent_files",
        nargs="+",  # Accept one or more files
        help="Path(s) to torrent file(s)"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Show additional information"
    )
    parser.add_argument(
        "-o", "--output",
        help="Output file to write magnet links (one per line)"
    )

    args = parser.parse_args()

    magnet_links = []
    failed_files = []

    for torrent_file in args.torrent_files:
        torrent_path = Path(torrent_file)

        if not torrent_path.exists():
            print(f"Error: File '{torrent_path}' not found", file=sys.stderr)
            failed_files.append(torrent_file)
            continue

        if not torrent_path.is_file():
            print(f"Error: '{torrent_path}' is not a file", file=sys.stderr)
            failed_files.append(torrent_file)
            continue

        try:
            magnet_link = torrent_to_magnet(torrent_path)
            magnet_links.append(magnet_link)

            if args.verbose:
                print(f"Converted: {torrent_path} -> magnet link", file=sys.stderr)

        except Exception as e:
            print(f"Error converting {torrent_path}: {e}", file=sys.stderr)
            failed_files.append(torrent_file)

    # Output results
    if args.output:
        # Write to output file
        try:
            with open(args.output, 'w') as f:
                for link in magnet_links:
                    f.write(link + '\n')
            if args.verbose:
                print(f"\nMagnet links written to: {args.output}", file=sys.stderr)
        except Exception as e:
            print(f"Error writing to output file: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        # Print to stdout
        for link in magnet_links:
            print(link)

    # Summary in verbose mode
    if args.verbose:
        print(f"\nSummary: {len(magnet_links)} successful, {len(failed_files)} failed", file=sys.stderr)

    # Exit with error if any files failed
    if failed_files:
        sys.exit(1)


if __name__ == "__main__":
    main()
