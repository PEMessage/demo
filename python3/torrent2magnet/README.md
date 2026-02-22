# torrent2magnet

Convert torrent files to magnet links.

## Installation

```bash
uv add torrent2magnet
```

Or install globally:
```bash
uv tool install torrent2magnet
```

## Usage

```bash
torrent2magnet path/to/torrent.torrent
```

Example:
```bash
$ torrent2magnet ubuntu-24.04.torrent
magnet:?xt=urn:btih:abc123...&tr=http://tracker.example.com:80/announce&dn=ubuntu-24.04.iso
```

### Options

- `-v, --verbose`: Show additional information
- `-h, --help`: Show help message

## Development

Install dependencies:
```bash
uv sync
```

Run tests:
```bash
uv run pytest
```

Install in development mode:
```bash
uv pip install -e .
```