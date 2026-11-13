#!/usr/bin/env python3
"""Create a controlled, experimentally Mupen-oriented libdragon IPL3 r4 ROM.

Only the IPL3 boot-code window at [0x40, 0x1000) is changed. All other ROM
bytes, including the header, metadata, and N64 application payload, stay the
same. This is a diagnostic compatibility experiment, not a game port and not
proof that any particular RetroArch build will execute it.

Source: DragonMinded/libdragon, commit 066e5fc39e, boot/bin/ipl3_prod.z64.
The 16-byte MD5 pin below is the r4 digest documented in that revision's
boot/README.md; this value authenticates the historical boot image to the
extent of MD5's non-cryptographic security (the Git commit also pins source).
"""

import argparse
import hashlib
from pathlib import Path
import sys

N64_Z64_MAGIC = bytes.fromhex("80371240")
BOOT_BEGIN = 0x40
BOOT_END = 0x1000
OLD_IPL3_R4_MD5 = "d78ddd06d2303d3f76d11de6b6326d69"
REFERENCE_PHASE6_SHA256 = "f4448625206ca37698681b4949cd538dcf5628737d8f14e5f3c12c83e0d185de"


def digest(data: bytes, algorithm: str) -> str:
    return hashlib.new(algorithm, data).hexdigest()


def splice_ipl3(original: bytes, historical_boot_image: bytes, *, verify_reference: bool = True) -> bytes:
    """Splice the historic IPL3 region into the otherwise unmodified ROM.

    Throws ValueError on a wrong/corrupt input. The function never patches the
    image in place. verify_reference=False is reserved for synthetic unit tests.
    """
    if len(original) < BOOT_END or original[:4] != N64_Z64_MAGIC:
        raise ValueError("Input must be a big-endian .z64 ROM of at least 4096 bytes")
    if len(historical_boot_image) < BOOT_END or historical_boot_image[:4] != N64_Z64_MAGIC:
        raise ValueError("Historical IPL3 must contain a signed big-endian N64 header")
    if verify_reference:
        if digest(original, "sha256") != REFERENCE_PHASE6_SHA256:
            raise ValueError("ROM SHA-256 differs from the verified Phase 6 diagnostic; refusing to modify")
        if digest(historical_boot_image, "md5") != OLD_IPL3_R4_MD5:
            raise ValueError("Historical IPL3 is not the pinned libdragon r4 image")
    modified = original[:BOOT_BEGIN] + historical_boot_image[BOOT_BEGIN:BOOT_END] + original[BOOT_END:]
    if len(modified) != len(original):
        raise AssertionError("ROM length unexpectedly changed")
    if modified[:BOOT_BEGIN] != original[:BOOT_BEGIN] or modified[BOOT_END:] != original[BOOT_END:]:
        raise AssertionError("Data outside IPL3 window unexpectedly changed")
    if modified[BOOT_BEGIN:BOOT_END] == original[BOOT_BEGIN:BOOT_END]:
        raise ValueError("IPL3 payload is identical; the experiment would not change any bytes")
    return modified


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, help="Verified Phase 6 diagnostic ROM (.z64)")
    parser.add_argument("historical_ipl3", type=Path, help="Pinned libdragon IPL3 r4 ROM image")
    parser.add_argument("output", type=Path, help="Experimental Mupen boot compatibility ROM")
    args = parser.parse_args()
    if args.output.resolve() in (args.rom.resolve(), args.historical_ipl3.resolve()):
        parser.error("Output must not overwrite an input file")
    try:
        original = args.rom.read_bytes()
        historical = args.historical_ipl3.read_bytes()
        output = splice_ipl3(original, historical)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(output)
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    print(f"Source ROM SHA-256: {digest(original,'sha256')}")
    print(f"Old IPL3 r4 MD5:    {digest(historical,'md5')}")
    print(f"Patched ROM SHA-256:{digest(output,'sha256')}")
    print(f"ROM size:           {len(output)} bytes")
    print("Patched only 0x40..0xFFF. Emulator compatibility remains unverified.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
