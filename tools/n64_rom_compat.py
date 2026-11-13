#!/usr/bin/env python3
"""Audit an N64 .z64 ROM and optionally make a size-controlled test copy.

This is a diagnostic tool, NOT an emulator compatibility patch.
Padding cannot repair an incorrect or unsupported IPL3 implementation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import zlib

MAGIC_Z64 = bytes.fromhex("80371240")
MAGIC_V64 = bytes.fromhex("37804012")
MAGIC_N64 = bytes.fromhex("40123780")
KNOWN_IPL3_CRC32 = {
    0x345E1229: "libdragon IPL3 r8 (reported incompatible with some Mupen64Plus cores)",
    0x86472013: "libdragon IPL3 r10 (reported incompatible with some Mupen64Plus cores)",
}
MIN_SIZE_TEST = 1024 * 1024


def inspect(data: bytes) -> dict:
    if len(data) < 0x1000:
        raise ValueError("ROM is smaller than 4096 bytes: missing full IPL3 region")
    if data[:4] != MAGIC_Z64:
        variant = ("byte-swapped .v64" if data[:4] == MAGIC_V64
                   else "word-swapped .n64" if data[:4] == MAGIC_N64
                   else "invalid N64 header")
        raise ValueError(f"Expected big-endian .z64 header, got {variant}")
    boot_crc32 = zlib.crc32(data[0x40:0x1000])
    return {
        "size_bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "title": data[0x20:0x34].rstrip(b"\0 ").decode("ascii", "replace"),
        "ipl3_prefix_crc32": f"{boot_crc32:08x}",
        "boot_family_hint": KNOWN_IPL3_CRC32.get(boot_crc32, "unidentified"),
        "elf_magic_offset": data.find(b"\x7fELF"),
        "toc0_offset": data.find(b"TOC0"),
        "size_below_1_mib": len(data) < MIN_SIZE_TEST,
        "runtime_verified": False,
    }


def padded_copy(original: bytes, min_length: int = MIN_SIZE_TEST) -> bytes:
    """Preserve all original ROM bytes; append zeroes only if shorter."""
    inspect(original)
    if min_length < len(original):
        raise ValueError("Requested minimum is smaller than original ROM")
    if min_length > 64 * MIN_SIZE_TEST:
        raise ValueError("Refusing a test ROM over 64 MiB")
    return original + bytes(min_length - len(original))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, help="Existing big-endian .z64 file")
    parser.add_argument("--pad-output", type=Path,
                        help="Write a 1 MiB minimum length test variant")
    args = parser.parse_args()
    data = args.rom.read_bytes()
    print(json.dumps(inspect(data), indent=2))
    if args.pad_output:
        output = padded_copy(data)
        args.pad_output.parent.mkdir(parents=True, exist_ok=True)
        args.pad_output.write_bytes(output)
        print(json.dumps({"padded_rom": str(args.pad_output),
                          "new_size": len(output),
                          "sha256": hashlib.sha256(output).hexdigest(),
                          "compatibility_fix": False}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
