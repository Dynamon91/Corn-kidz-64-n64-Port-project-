"""Host-side correctness tests; NOT tests of the N64 emulator boot path."""

import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from splice_ipl3_r4 import BOOT_BEGIN, BOOT_END, N64_Z64_MAGIC, splice_ipl3


def fixture(length=147456):
    raw = bytearray((i * 31 + 23) & 0xFF for i in range(length))
    raw[:4] = N64_Z64_MAGIC
    old = bytearray([0xA5] * 5992)
    old[:4] = N64_Z64_MAGIC
    return bytes(raw), bytes(old)


class CompatibilityPatchTests(unittest.TestCase):
    def test_replaces_only_boot_code_range(self):
        old_rom, boot = fixture()
        out = splice_ipl3(old_rom, boot, verify_reference=False)
        self.assertEqual(len(out), len(old_rom))
        self.assertEqual(out[:BOOT_BEGIN], old_rom[:BOOT_BEGIN])
        self.assertEqual(out[BOOT_BEGIN:BOOT_END], boot[BOOT_BEGIN:BOOT_END])
        self.assertEqual(out[BOOT_END:], old_rom[BOOT_END:])

    def test_wrong_byte_order_rejected(self):
        old_rom, boot = fixture()
        with self.assertRaises(ValueError):
            splice_ipl3(b'BAD!' + old_rom[4:], boot, verify_reference=False)

    def test_truncated_rom_rejected(self):
        _, boot = fixture()
        with self.assertRaises(ValueError):
            splice_ipl3(N64_Z64_MAGIC + b'0' * 100, boot, verify_reference=False)

    def test_truncated_historical_boot_rejected(self):
        old_rom, boot = fixture()
        with self.assertRaises(ValueError):
            splice_ipl3(old_rom, boot[:500], verify_reference=False)

    def test_exact_same_boot_rejected(self):
        old_rom, _ = fixture()
        with self.assertRaises(ValueError):
            splice_ipl3(old_rom, old_rom[:5992], verify_reference=False)

    def test_reference_digest_is_mandatory_by_default(self):
        old_rom, boot = fixture()
        with self.assertRaises(ValueError):
            splice_ipl3(old_rom, boot)


if __name__ == '__main__':
    unittest.main()
