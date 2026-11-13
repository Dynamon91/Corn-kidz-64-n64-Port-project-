"""Synthetic Unity Mesh fixtures only; no game meshes in source control."""
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from unity_mesh_probe import (parse_mesh,decode_positions,triangles,scene_external_paths,
                              ReadError,Cursor,_packed_int,_packed_float,encode_ckm1,decode_ckm1)


def u(n):return struct.pack('<I',n)
def aligned(data):return data+bytes((-len(data))%4)
def named(name):return aligned(u(len(name))+name.encode())


def fixture(*, indexed=True, vertex_count=3, topology=0, external=False,
            index_format=0, flags=0x34, invalid_sub_count=None,
            use_half=False, different_stream=False, bad_vertex_buffer=False):
    p=named('synthetic-triangle')
    subcnt=1 if invalid_sub_count is None else invalid_sub_count
    p+=u(subcnt)
    p+=struct.pack('<6I',0,3,topology,0,0,3)
    p+=struct.pack('<6f',0,0,0,1,1,1)
    # BlendShapeData four empty arrays, bones matrices/hashes, root, AABB, weights.
    p+=u(0)*4+u(0)*5
    p+=bytes([0,1,1,1])+u(index_format)
    indexes=struct.pack('<3H',0,1,2) if indexed and index_format==0 else (
        struct.pack('<3I',0,1,2) if indexed else b'')
    p+=aligned(u(len(indexes))+indexes)
    ch0=(0,0,1 if use_half else 0,3)
    ch1=(1,0,0,3) if different_stream else (0,0,0,0)
    channels=[bytes(ch0),bytes(ch1)]+[b'\0'*4]*12
    p+=u(vertex_count)+u(14)+b''.join(channels)
    points=[(1.0,2.0,3.0),(4.0,5.0,6.0),(-2.0,0.5,0.0)]
    vblob=b''.join(struct.pack('<3e' if use_half else '<3f',*x) for x in points)
    if different_stream:
        vblob=aligned(vblob)+b'\0'* (16*3)+b'\0'* (3*3*4)
    if bad_vertex_buffer:vblob=vblob[:2]
    p+=aligned(u(0 if external else len(vblob))+(b'' if external else vblob))
    # Four PackedFloatVector, six PackedInt/Float in Unity 2021 order.
    for kind in ('f','f','f','f','i','i','i','f','i','i'):
        p+=u(0)*4+b'\0'*4 if kind=='f' else u(0)*2+b'\0'*4
    p+=u(0)  # UVInfo
    p+=struct.pack('<6f',0,0,0,1,1,1)
    p+=u(0)+u(0)+u(0)+struct.pack('<2f',0.0,0.0)
    p+=struct.pack('<QI',0,len(vblob) if external else 0)
    p+=named('sharedassets2.assets.resS' if external else '')
    return p,vblob


class MeshTests(unittest.TestCase):
    def test_fixture_mesh_header(self):
        data,_=fixture()
        m=parse_mesh(data)
        self.assertEqual(m['name'],'synthetic-triangle')
        self.assertEqual(len(m['submeshes']),1)
        self.assertEqual(m['vertex_count'],3)
        self.assertEqual(len(m['indices']),3)
        self.assertEqual(m['index_format'],0)

    def test_position_triples(self):
        m=parse_mesh(fixture()[0]);v=decode_positions(m,m['vertex_blob'])
        self.assertEqual(v,[(1.,2.,3.),(4.,5.,6.),(-2.,0.5,0.)])

    def test_triangle_indexing(self):
        m=parse_mesh(fixture()[0]);v=decode_positions(m,m['vertex_blob'])
        self.assertEqual(triangles(m,v),[(0,1,2)])

    def test_nontriangle_topology_is_not_invented(self):
        m=parse_mesh(fixture(topology=3)[0]);self.assertEqual(triangles(m,decode_positions(m,m['vertex_blob'])),[])

    def test_index_format_32_bit(self):
        m=parse_mesh(fixture(index_format=1)[0]);self.assertEqual(m['indices'],[0,1,2])

    def test_channel_dimension_mask(self):
        # Actual Unity 2021 export has dimension byte 0x34, low nibble 4.
        m=parse_mesh(fixture()[0]);self.assertEqual(m['channels'][0][3],3)
        raw=bytearray(fixture()[0]);at=raw.find(u(3)+u(14)+bytes([0,0,0,3]))+8
        self.assertNotEqual(at,-1)
        raw[at+3]=0x33
        self.assertEqual(parse_mesh(bytes(raw))['channels'][0][3],3)

    def test_half_precision_position(self):
        m=parse_mesh(fixture(use_half=True)[0]);self.assertEqual(decode_positions(m,m['vertex_blob'])[2],(-2.,0.5,0.))

    def test_external_stream_metadata(self):
        raw,external=fixture(external=True)
        m=parse_mesh(raw)
        self.assertEqual(m['vertex_blob'],b'')
        self.assertTrue(m['stream_path'].endswith('.resS'))
        self.assertEqual(m['stream_size'],len(external))
        self.assertEqual(decode_positions(m,external)[0],(1.,2.,3.))

    def test_reject_vertex_truncation(self):
        raw,_=fixture(bad_vertex_buffer=True)
        m=parse_mesh(raw)
        with self.assertRaisesRegex(ReadError,'stream too small'):
            decode_positions(m,m['vertex_blob'])

    def test_zero_vertex_count_rejected_for_indexed_mesh(self):
        raw,_=fixture(vertex_count=0)
        m=parse_mesh(raw)
        with self.assertRaises(ReadError):triangles(m,decode_positions(m,b''))

    def test_reject_index_outside_buffer(self):
        raw,_=fixture(indexed=False)
        with self.assertRaisesRegex(ReadError,'outside buffer'):
            parse_mesh(raw)

    def test_reject_malformed_mesh_count(self):
        raw,_=fixture(invalid_sub_count=5000)
        with self.assertRaisesRegex(ReadError,'invalid submesh count'):
            parse_mesh(raw)

    def test_reject_truncated_mesh(self):
        raw,_=fixture()
        with self.assertRaises(ReadError):parse_mesh(raw[:-5])

    def test_reject_unexpected_trailing_bytes(self):
        raw,_=fixture()
        with self.assertRaisesRegex(ReadError,'trailing bytes'):parse_mesh(raw+b'xxxx')

    def test_packed_empty_arrays(self):
        r=Cursor(u(0)*4+b'\0'*4)
        self.assertEqual(_packed_float(r),0)
        r=Cursor(u(0)*2+b'\0'*4)
        self.assertEqual(_packed_int(r),0)

    def test_external_file_id_order(self):
        prefix=b' '*60+b'globalgamemanagers.assets\0\x00\x00sharedassets1.assets\0\x00\x00sharedassets3.assets\0'
        meta=bytearray(prefix+b'x'*20)
        struct.pack_into('>Q',meta,32,len(meta))
        self.assertEqual(scene_external_paths(meta),{1:'globalgamemanagers.assets',2:'sharedassets1.assets',3:'sharedassets3.assets'})


    def test_ckm1_roundtrip(self):
        m=parse_mesh(fixture()[0]);v=decode_positions(m,m['vertex_blob']);f=triangles(m,v)
        blob=encode_ckm1(v,f);rec,faces=decode_ckm1(blob)
        self.assertEqual(len(blob),28+(len(v)+len(f))*6)
        self.assertEqual(faces,f)
        for a,b in zip(v,rec):
            self.assertTrue(all(abs(x-y)<0.001 for x,y in zip(a,b)))

    def test_ckm1_reject_corruption(self):
        b=encode_ckm1([(0.,0.,0.),(1.,0.,0.),(0.,1.,0.)],[(0,1,2)])
        with self.assertRaises(ReadError): decode_ckm1(b[:-2])
        with self.assertRaises(ReadError): decode_ckm1(b'BAD!'+b[4:])
        with self.assertRaises(ReadError): decode_ckm1(b[:50]+b'\xff\xff')

    def test_ckm1_reject_out_of_range(self):
        with self.assertRaises(ReadError): encode_ckm1([(0,0,0)],[(0,0,1)])
        with self.assertRaises(ReadError): encode_ckm1([],[])

    def test_ckm1_uses_big_endian_indices(self):
        b=encode_ckm1([(0,0,0),(1,0,0),(0,1,0)],[(0,1,2)])
        self.assertEqual(b[-6:],bytes.fromhex('000000010002'))

    def test_cannot_overread_cursor(self):
        r=Cursor(b'123')
        with self.assertRaises(ReadError):r.u32()
        with self.assertRaises(ReadError):r.consume(-1)

if __name__=='__main__':unittest.main()
