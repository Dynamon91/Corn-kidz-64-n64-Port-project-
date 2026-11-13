"""Host-side tests; no emulator or real N64 execution is implied."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from n64_rom_compat import inspect, padded_copy, MIN_SIZE_TEST  # noqa: E402


def make_rom(size=4096):
    b = bytearray(size)
    b[:4] = bytes.fromhex('80371240')
    b[0x20:0x20 + 10] = b'CK64 TEST\0'
    b[0x100:0x104] = b'TOC0'
    return bytes(b)


class RomAuditTests(unittest.TestCase):
    def test_valid_header(self):
        result = inspect(make_rom())
        self.assertEqual(result['size_bytes'], 4096)
        self.assertEqual(result['toc0_offset'], 0x100)
        self.assertFalse(result['runtime_verified'])

    def test_reject_wrong_endian(self):
        for magic in (bytes.fromhex('37804012'), bytes.fromhex('40123780')):
            b = bytearray(make_rom())
            b[:4] = magic
            with self.assertRaises(ValueError):
                inspect(bytes(b))

    def test_reject_truncated(self):
        with self.assertRaises(ValueError):
            inspect(make_rom()[:350])

    def test_padding_preserves_original(self):
        original = make_rom(8192)
        padded = padded_copy(original)
        self.assertEqual(len(padded), MIN_SIZE_TEST)
        self.assertEqual(padded[:len(original)], original)
        self.assertEqual(padded[len(original):], bytes(MIN_SIZE_TEST - len(original)))

    def test_reject_shrink(self):
        with self.assertRaises(ValueError):
            padded_copy(make_rom(9000), 8000)

    def test_reject_excessive(self):
        with self.assertRaises(ValueError):
            padded_copy(make_rom(), 65*MIN_SIZE_TEST)


if __name__ == '__main__':
    unittest.main()
