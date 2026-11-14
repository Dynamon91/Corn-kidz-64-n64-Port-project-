"""Synthetic-only checks for Phase 23 Transform selection and private header output."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from unity_mesh_probe import encode_ckm1
from unity_phase23_export import private_header, resolve_instance


def transform(go_id, parent=0, pos=(0, 0, 0), rot=(0, 0, 0, 1), scale=(1, 1, 1)):
    b = bytearray(80)
    struct.pack_into("<iq", b, 0, 0, go_id)
    struct.pack_into("<4f", b, 12, *rot)
    struct.pack_into("<3f", b, 28, *pos)
    struct.pack_into("<3f", b, 40, *scale)
    struct.pack_into("<i", b, 52, 0)
    struct.pack_into("<iq", b, 56, 0, parent)
    return bytes(b)


def mesh_filter(go_id, file_id, path_id):
    return struct.pack("<iqiq", 0, go_id, file_id, path_id)


def game_object(transform_id):
    b = bytearray(4 + 12 + 8 + 1)
    struct.pack_into("<i", b, 0, 1)
    struct.pack_into("<iq", b, 4, 0, transform_id)
    struct.pack_into("<ii", b, 16, 0, 1)
    b[24] = 0
    return bytes(b)


class Phase23ExportTests(unittest.TestCase):
    def test_world_matrix_follows_parent_scale(self):
        records = [(11, 4, transform(1, pos=(10, 0, 0), scale=(2, 2, 2))),
                   (12, 4, transform(2, parent=11, pos=(1, 0, 0))),
                   (1, 1, game_object(11)), (2, 1, game_object(12)),
                   (21, 33, mesh_filter(2, 3, 194))]
        matrix, count = resolve_instance(records, "3:194")
        self.assertEqual(count, 1)
        self.assertAlmostEqual(matrix[0][3], 12.0)
        self.assertAlmostEqual(matrix[1][3], 0.0)

    def test_export_header_has_matrix_and_private_marker(self):
        blob = encode_ckm1([(0, 0, 0), (1, 0, 0), (0, 1, 0)], [(0, 1, 2)])
        matrix = ((1, 0, 0, 4), (0, 1, 0, 5), (0, 0, 1, 6))
        header = private_header(blob, matrix, 1)
        self.assertIn("PRIVATE COMMERCIAL GAME DERIVATIVE", header)
        self.assertIn("4.0f", header)
        self.assertIn("CK64_PHASE23_SOURCE_TRIANGLES 1u", header)
        self.assertIn("0x43, 0x4b, 0x4d, 0x31", header)

    def test_instance_index_is_bounded(self):
        records = [(11, 4, transform(1)), (1, 1, game_object(11)),
                   (21, 33, mesh_filter(1, 3, 194))]
        with self.assertRaisesRegex(ValueError, "outside"):
            resolve_instance(records, "3:194", 1)

    def test_resolved_instance_can_skip_other_unresolved_placement(self):
        records = [
            (11, 4, transform(1, parent=99)),
            (12, 4, transform(2, pos=(7, 0, 0))),
            (1, 1, game_object(11)),
            (2, 1, game_object(12)),
            (21, 33, mesh_filter(1, 3, 194)),
            (22, 33, mesh_filter(2, 3, 194)),
        ]
        matrix, count = resolve_instance(records, "3:194", 1)
        self.assertEqual(count, 2)
        self.assertAlmostEqual(matrix[0][3], 7.0)
        with self.assertRaisesRegex(ValueError, "unresolved"):
            resolve_instance(records, "3:194", 0)

    def test_unresolved_transform_rejected(self):
        records = [(11, 4, transform(1, parent=99)), (1, 1, game_object(11)),
                   (21, 33, mesh_filter(1, 3, 194))]
        with self.assertRaisesRegex(ValueError, "unresolved"):
            resolve_instance(records, "3:194")

    def test_bad_mesh_reference_rejected(self):
        records = [(11, 4, transform(1)), (1, 1, game_object(11)),
                   (21, 33, mesh_filter(1, 3, 194))]
        with self.assertRaisesRegex(ValueError, "no scene instance"):
            resolve_instance(records, "4:194")


if __name__ == "__main__":
    unittest.main()
