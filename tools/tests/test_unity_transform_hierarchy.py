"""Synthetic Unity fixtures only; no proprietary assets committed."""
import math
import json
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from unity_transform_hierarchy import (read_transform,read_mesh_filter,local_matrix,
                                       product,build_hierarchy,summarize_scene)


def transform(go, xyz=(0,0,0), parent=0, rot=(0,0,0,1), scl=(1,1,1), kids=()):
    return (struct.pack('<iq4f3f3fi',0,go,*rot,*xyz,*scl,len(kids))+
            b''.join(struct.pack('<iq',0,k) for k in kids)+
            struct.pack('<iq',0,parent))


def mesh(go,file=3,ref=185):
    return struct.pack('<iqiq',0,go,file,ref)


def go(name,component):
    b=name.encode()
    return (struct.pack('<i',1)+struct.pack('<iq',0,component)+
            struct.pack('<i',0)+struct.pack('<i',len(b))+b+
            bytes((-len(b))%4)+b'\0\0\1')

class TransformTests(unittest.TestCase):
    def test_offsets_and_parent(self):
        t=read_transform(transform(1,(1,2,3),44))
        self.assertEqual(t['position'],(1,2,3))
        self.assertEqual(t['parent'],(0,44))
        self.assertEqual(t['rotation'],(0,0,0,1))
    def test_children(self):
        t=read_transform(transform(1,kids=(7,8)))
        self.assertEqual(t['children'],[(0,7),(0,8)])
    def test_identity_matrix(self):
        t=read_transform(transform(1,(2,3,4),scl=(2,3,4)))
        m=local_matrix(t)
        self.assertEqual(tuple(row[3] for row in m),(2,3,4))
        self.assertEqual((m[0][0],m[1][1],m[2][2]),(2,3,4))
    def test_rotation_y_90(self):
        s=math.sqrt(0.5)
        t=read_transform(transform(1,(1,2,3),rot=(0,s,0,s)))
        m=local_matrix(t)
        self.assertAlmostEqual(m[0][2],1.0)
        self.assertAlmostEqual(m[2][0],-1.0)
    def test_matrix_product_rotated_child(self):
        s=math.sqrt(0.5)
        records=[(1,1,go('player',100)),(2,1,go('parent',101)),
                 (100,4,transform(1,(0,0,2),101)),
                 (101,4,transform(2,(10,0,20),rot=(0,s,0,s),scl=(2,1,2)))]
        graph=build_hierarchy(records)
        mat=graph['world_matrices'][100]
        self.assertAlmostEqual(mat[0][3],14,places=4)
        self.assertAlmostEqual(mat[1][3],0,places=4)
        self.assertAlmostEqual(mat[2][3],20,places=4)
    def test_nonuniform_scale_child_order(self):
        root=local_matrix(read_transform(transform(1,(5,0,0),scl=(2,3,4))))
        child=local_matrix(read_transform(transform(2,(1,1,1))))
        mat=product(root,child)
        self.assertEqual(tuple(row[3] for row in mat),(7,3,4))
    def test_cycle_rejected(self):
        records=[(11,4,transform(1,parent=12)),(12,4,transform(2,parent=11))]
        with self.assertRaisesRegex(ValueError,'cycle'):build_hierarchy(records)
    def test_ui_parent_unresolved(self):
        records=[(1,1,go('checkpoint',11)),(11,4,transform(1,(1,2,3),parent=99)),
                 (99,224,b'')]
        g=build_hierarchy(records)
        self.assertIsNone(g['world_matrices'][11])
        self.assertEqual(g['statuses'][11],'unsupported_rect_transform')
    def test_children_inherit_unresolved(self):
        records=[(11,4,transform(1,parent=99)),(12,4,transform(2,parent=11)),
                 (99,224,b'')]
        g=build_hierarchy(records)
        self.assertIsNone(g['world_matrices'][12])
        self.assertEqual(g['statuses'][12],'unsupported_rect_transform')
    def test_missing_parent(self):
        g=build_hierarchy([(11,4,transform(1,parent=99))])
        self.assertEqual(g['statuses'][11],'unresolved_parent')
    def test_external_parent(self):
        b=bytearray(transform(1,parent=99))
        struct.pack_into('<i',b,len(b)-12,1)
        g=build_hierarchy([(11,4,bytes(b))])
        self.assertEqual(g['statuses'][11],'external_parent')
    def test_mesh_filter_external_pointer(self):
        self.assertEqual(read_mesh_filter(mesh(33,6,10209)),
                         {'game_object_id':33,'mesh_file_id':6,'mesh_path_id':10209})
    def test_mesh_filter_size_validation(self):
        with self.assertRaisesRegex(ValueError,'layout'):read_mesh_filter(b'123')
    def test_invalid_quaternion_rejected(self):
        with self.assertRaisesRegex(ValueError,'zero quaternion'):
            read_transform(transform(1,rot=(0,0,0,0)))
        with self.assertRaisesRegex(ValueError,'nonfinite'):
            read_transform(transform(1,xyz=(0,float('nan'),0)))
    def test_bad_children_rejected(self):
        b=bytearray(transform(1));struct.pack_into('<i',b,52,1000)
        with self.assertRaisesRegex(ValueError,'children count'):read_transform(bytes(b))
    def test_metadata_header_matches_source_positions(self):
        root = Path(__file__).resolve().parents[2]
        doc = json.loads((root/'reports/phase21_tutorial_anchors.json').read_text())
        self.assertEqual(doc['scene'],'TutorialWorld01')
        header = (root/'n64/scene_anchor_meta.h').read_text()
        for name, macro in (('player','CK64_TUTORIAL_PLAYER_WORLD'),
                            ('spawnPoint','CK64_TUTORIAL_SPAWN_WORLD')):
            anchor = next(item for item in doc['anchors'] if item['name']==name)
            x,y,z = anchor['world_position']
            label='PLAYER' if name=='player' else 'SPAWN'
            literal=f'{label} XYZ: {x:.2f} {y:.2f} {z:.2f}'
            self.assertIn(f'#define {macro} "{literal}"',header)

    def test_summary_world_anchor_and_file_counts(self):
        records=[(1,1,go('player',11)),(2,1,go('checkpoint',12)),
                 (11,4,transform(1,(1,2,3))),
                 (12,4,transform(2,(4,5,6),parent=11)),
                 (20,33,mesh(1,4,222)),(21,33,mesh(2,4,333))]
        s=summarize_scene(records,'fixture',3)
        self.assertEqual(s['native_transforms'],2)
        self.assertEqual(s['mesh_filters'],2)
        self.assertEqual(s['mesh_reference_file_id_counts'],{'4':2})
        self.assertEqual(s['anchors'][0]['world_position'],[5,7,9])
        self.assertFalse(s['geometry_extracted'])

if __name__=='__main__':unittest.main()
