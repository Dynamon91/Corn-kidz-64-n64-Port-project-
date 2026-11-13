"""Synthetic fixtures only — never commit proprietary Unity data."""
import sys
import struct
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from unity_scene_inventory import (parse_serialized, game_object, transform, scan_scene,
                                   scene_names)


def gobj(name, path):
    n=name.encode()
    return (struct.pack('<i',1)+struct.pack('<iq',0,path)+struct.pack('<i',0)
            +struct.pack('<i',len(n))+n+bytes((-len(n))%4)+b'\0\0\x01')


def tf(go, pos, parent=0):
    return (struct.pack('<iq',0,go)+struct.pack('<4f',0,0,0,1)
            +struct.pack('<3f',*pos)+struct.pack('<3f',1,1,1)
            +struct.pack('<i',0)+struct.pack('<iq',0,parent))


def fixture(objects=None,version=22,typetree=0):
    if objects is None:
        objects=[(1,0,gobj('player',2)),(2,1,tf(1,(1.0,2.5,-3.0)))]
    names=[1,4]
    meta=bytearray(b'\0'*48+b'2021.3.30f1\0'+struct.pack('<i',19)
                   +bytes((typetree,))+struct.pack('<i',len(names)))
    for class_id in names:
        meta += struct.pack('<ibh',class_id,0,-1)+b'\0'*16
    meta += struct.pack('<i',len(objects))
    meta += bytes((-len(meta))%4)
    data=bytearray()
    for path,typeidx,blob in objects:
        meta += struct.pack('<qqIi',path,len(data),len(blob),typeidx)
        data+=blob
        data+=bytes((-len(data))%8)
    offset=(len(meta)+15)//16*16
    meta+=bytes(offset-len(meta))
    struct.pack_into('>I',meta,8,version)
    struct.pack_into('>IQQ',meta,20,offset-1,offset+len(data),offset)
    return bytes(meta+data)

class UnitySceneTests(unittest.TestCase):
    def test_scene_objects_and_local_positions(self):
        rows,meta=parse_serialized(fixture())
        self.assertEqual(meta['object_count'],2)
        self.assertEqual([x[1] for x in rows],[1,4])
        self.assertEqual(game_object(rows[0][2])['name'],'player')
        self.assertEqual(transform(rows[1][2])['local_position'],[1.0,2.5,-3.0])
    def test_named_anchors_are_not_world_coordinates(self):
        scan=scan_scene(fixture(),'Assets/Scenes/TutorialWorld01.unity')
        self.assertEqual(scan['class_counts']['GameObject'],1)
        self.assertEqual(scan['anchors'],[{'name':'player','local_position':[1.0,2.5,-3.0],'has_parent':False}])
    def test_non_anchors_excluded(self):
        sample=fixture([(9,0,gobj('temporary decorative mesh',10)),(10,1,tf(9,(0,0,1)))])
        self.assertEqual(scan_scene(sample,'x')['anchors'],[])
        self.assertEqual(len(scan_scene(sample,'x',include_all=True)['anchors']),1)
    def test_parent_transform_flag(self):
        sample=fixture([(1,0,gobj('checkpoint',2)),(2,1,tf(1,(1,2,3),parent=44))])
        self.assertTrue(scan_scene(sample,'x')['anchors'][0]['has_parent'])
    def test_build_paths_order(self):
        data=b'irrelevant Assets/Scenes/title01.unity\0garbage Assets/Scenes/Wolloh\'s Hollow.unity\0'+b'Assets/Scenes/title01.unity'
        self.assertEqual(scene_names(data),["Assets/Scenes/title01.unity", "Assets/Scenes/Wolloh's Hollow.unity"])
    def test_wrong_version_and_tree_rejected(self):
        with self.assertRaisesRegex(ValueError,'version'):
            parse_serialized(fixture(version=21))
        with self.assertRaisesRegex(ValueError,'type-tree'):
            parse_serialized(fixture(typetree=1))
    def test_truncated_and_tampered_rejected(self):
        data=fixture()
        with self.assertRaises(ValueError):parse_serialized(data[:-5])
        b=bytearray(data)
        struct.pack_into('<i',b,48+len('2021.3.30f1')+1+4+1,99999)
        with self.assertRaises(ValueError):parse_serialized(bytes(b))
    def test_invalid_components_and_transform(self):
        self.assertIsNone(game_object(b'\xff'*100))
        self.assertIsNone(transform(b'\0'*30))
        self.assertIsNone(transform(tf(1,(float('nan'),0,0))))

if __name__=='__main__':unittest.main()