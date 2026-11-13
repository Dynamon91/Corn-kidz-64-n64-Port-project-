#!/usr/bin/env python3
"""Verify a sequence of split ZIP byte chunks and export a safe file manifest.

Example:
    python3 tools/scan_split_zip.py '/path/Corn.Kidz.64.v1.029 (1).zip.001' \
        --manifest reports/archive_manifest.json

Requires Python 3.9+ only. Never runs executables from the supplied archive.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import tempfile
import zipfile
from pathlib import Path


def find_parts(first_part: Path) -> list[Path]:
    match = re.fullmatch(r'(.+\.zip)\.(\d{3})', first_part.name, re.I)
    if not match or match.group(2) != '001':
        raise ValueError('Expected the .zip.001 first part')
    base_name = match.group(1)
    parts = sorted(first_part.parent.glob(base_name + '.[0-9][0-9][0-9]'))
    if not parts or parts[0].name != first_part.name:
        raise ValueError('First part missing')
    for index, part in enumerate(parts, 1):
        if part.name != f'{base_name}.{index:03d}':
            raise ValueError(f'Missing/out-of-order part before {part.name}')
    return parts


def combine_and_scan(parts: list[Path], check_crc: bool) -> dict:
    sha_total = hashlib.sha256()
    part_stats = []
    with tempfile.TemporaryDirectory(prefix='split_zip_inspect_') as temp:
        combined = Path(temp) / 'combined.zip'
        with combined.open('wb') as dest:
            for part in parts:
                sha_part = hashlib.sha256()
                with part.open('rb') as source:
                    while True:
                        chunk = source.read(4 * 1024 * 1024)
                        if not chunk:
                            break
                        sha_part.update(chunk)
                        sha_total.update(chunk)
                        dest.write(chunk)
                part_stats.append({'part': part.name, 'size': part.stat().st_size,
                                   'sha256': sha_part.hexdigest()})
        with zipfile.ZipFile(combined) as archive:
            members = archive.infolist()
            entries = [{
                'name': m.filename, 'size': m.file_size, 'compressed_size': m.compress_size,
                'crc32': f'{m.CRC:08x}', 'is_directory': m.is_dir(),
            } for m in members]
            invalid_member = archive.testzip() if check_crc else None
            if invalid_member:
                raise ValueError(f'ZIP member failed CRC: {invalid_member}')
            stats = {
                'archive_bytes': combined.stat().st_size,
                'sha256': sha_total.hexdigest(),
                'entries': len(members),
                'regular_files': sum(not m.is_dir() for m in members),
                'uncompressed_bytes': sum(m.file_size for m in members),
                'crc_checked': check_crc,
                'crc_valid': (invalid_member is None) if check_crc else None,
            }
    return {'summary': stats, 'parts': part_stats, 'files': entries}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('first_part', type=Path)
    parser.add_argument('--manifest', type=Path, help='optional JSON output')
    parser.add_argument('--skip-crc', action='store_true', help='faster scan, skips CRC verification')
    args = parser.parse_args()
    parts = find_parts(args.first_part)
    result = combine_and_scan(parts, check_crc=not args.skip_crc)
    print(json.dumps(result['summary'], indent=2))
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(json.dumps(result, indent=2), encoding='utf-8')
        print('Wrote:', args.manifest)


if __name__ == '__main__':
    main()
