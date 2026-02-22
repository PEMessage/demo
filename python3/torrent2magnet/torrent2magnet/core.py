"""Core functionality for converting torrent files to magnet links"""

import hashlib
import urllib.parse
from pathlib import Path
from typing import Dict, Any, Optional
import bencode

def read_torrent_file(torrent_path: Path) -> Dict[str, Any]:
    """Read and parse a torrent file"""
    with open(torrent_path, 'rb') as f:
        torrent_data = f.read()

    return bencode.decode(torrent_data)

def calculate_info_hash(torrent_info: Dict[str, Any]) -> str:
    """Calculate the SHA-1 hash of the info dictionary"""
    encoded_info = bencode.encode(torrent_info)
    sha1_hash = hashlib.sha1(encoded_info).digest()
    return sha1_hash.hex()

def get_trackers(torrent_data: Dict[str, Any]) -> list:
    """Extract tracker URLs from torrent data"""
    trackers = []

    # Single tracker (announce)
    if 'announce' in torrent_data:
        trackers.append(torrent_data['announce'])

    # Multiple trackers (announce-list)
    if 'announce-list' in torrent_data:
        for tracker_group in torrent_data['announce-list']:
            for tracker in tracker_group:
                trackers.append(tracker)

    # Remove duplicates while preserving order
    seen = set()
    unique_trackers = []
    for tracker in trackers:
        if tracker not in seen:
            seen.add(tracker)
            unique_trackers.append(tracker)

    return unique_trackers

def get_name(torrent_info: Dict[str, Any]) -> Optional[str]:
    """Get the name from torrent info"""
    if 'name' in torrent_info:
        return torrent_info['name']
    return None

def build_magnet_link(
    info_hash: str,
    trackers: list,
    name: Optional[str] = None
) -> str:
    """Build a magnet link from components"""
    magnet_parts = [f"magnet:?xt=urn:btih:{info_hash}"]

    # Add trackers
    for tracker in trackers:
        encoded_tracker = urllib.parse.quote(tracker, safe='')
        magnet_parts.append(f"tr={encoded_tracker}")

    # Add name if available
    if name:
        encoded_name = urllib.parse.quote(name, safe='')
        magnet_parts.append(f"dn={encoded_name}")

    return "&".join(magnet_parts)

def torrent_to_magnet(torrent_path: Path) -> str:
    """Convert a torrent file to a magnet link"""
    # Read and parse torrent file
    torrent_data = read_torrent_file(torrent_path)

    # Get info dictionary
    if 'info' not in torrent_data:
        raise ValueError("Torrent file missing 'info' section")

    torrent_info = torrent_data['info']

    # Calculate info hash
    info_hash = calculate_info_hash(torrent_info)

    # Get trackers
    trackers = get_trackers(torrent_data)

    # Get name
    name = get_name(torrent_info)

    # Build magnet link
    return build_magnet_link(info_hash, trackers, name)
