#!/usr/bin/env python3
"""Inspect Unity 2021.3 SerializedFile v22 Mesh geometry in an *owned* game ZIP.

This independent stdlib decoder is purpose-built for the observed Unity 2021
Mesh (class 43) layout. It reads submesh index buffers, vertex channel 0 and
external .resS streams; it does not decode textures/materials, terrain,
colliders, morphs or original animation. Never publish raw game geometry.

Public CLI: python tools/unity_mesh_probe.py owned.zip --scene 3 --summary report.json
Private CLI: ... --private-obj /private/one_mesh.obj --mesh-id 3:179
The OBJ is an export of proprietary user-supplied content, not a redistributable
project asset. The mesh-id is scene-fileID:meshPathID.
"""
from __future__ import annotations
import argparse
import collections
import hashlib
import json
import math
import re
import struct
import zipfile
from pathlib import Path

from unity_scene_inventory import parse_serialized, scene_names
from unity_transform_hierarchy import build_hierarchy

MAX_COUNT = 3000000


class ReadError(ValueError):
    pass


class Cursor:
    def __init__(self, data: bytes):
        self.buf = memoryview(data)
        self.pos = 0
    def consume(self, size):
        if not isinstance(size, int) or size < 0 or self.pos + size > len(self.buf):
            raise ReadError('invalid size / buffer overrun')
        p = self.pos
        self.pos += size
        return self.buf[p:p+size]
    def u32(self):
        return struct.unpack('<I', self.consume(4))[0]
    def i32(self):
        return struct.unpack('<i', self.consume(4))[0]
    def u64(self):
        return struct.unpack('<Q', self.consume(8))[0]
    def f32(self):
        return struct.unpack('<f', self.consume(4))[0]
    def align(self, n=4):
        self.consume((-self.pos) % n)
    def string(self):
        n = self.i32()
        if n < 0 or n > 4096:
            raise ReadError('invalid string length')
        s = self.consume(n).tobytes().decode('utf8', 'replace')
        self.align()
        return s
    def counted(self, item_size):
        n = self.i32()
        if n < 0 or n > MAX_COUNT:
            raise ReadError('array element count out of bounds')
        self.consume(n*item_size)
        return n
    def counted_bytes(self):
        n = self.i32()
        if n < 0 or n > 128*1024*1024:
            raise ReadError('byte array size out of bounds')
        b = self.consume(n).tobytes()
        self.align()
        return b


def _packed_float(r):
    items = r.u32()
    r.consume(8)  # range/start
    size = r.i32()
    if size < 0 or size > 64*1024*1024:
        raise ReadError('packed float byte count invalid')
    r.consume(size)
    r.align()
    bits = int(r.consume(1)[0])
    r.align()
    if bits > 32:
        raise ReadError('packed float bits invalid')
    return items


def _packed_int(r):
    items = r.u32()
    size = r.i32()
    if size < 0 or size > 64*1024*1024:
        raise ReadError('packed int byte count invalid')
    r.consume(size)
    r.align()
    bits = int(r.consume(1)[0])
    r.align()
    if bits > 32:
        raise ReadError('packed int bits invalid')
    return items


def parse_mesh(body):
    """Return strict Mesh header + index + vertex-stream location.

    Vertex bytes are decoded separately after resolving optional external data.
    The parser has explicit bounds, sanity, and EOF checks to reject accidental
    false-positive layouts.
    """
    r = Cursor(body)
    name = r.string()
    nsub = r.i32()
    if not 0 <= nsub <= 4096:
        raise ReadError('invalid submesh count')
    submeshes = []
    for _ in range(nsub):
        first, count, topology, base, start, nvert = (r.u32() for _ in range(6))
        center = [r.f32() for _ in range(3)]
        extent = [r.f32() for _ in range(3)]
        if not all(math.isfinite(v) for v in center+extent) or any(e < 0 for e in extent):
            raise ReadError('invalid submesh AABB')
        submeshes.append({'first_byte':first,'index_count':count,'topology':topology,
                          'base_vertex':base,'first_vertex':start,
                          'vertex_count':nvert,'center':center,'extent':extent})
    # BlendShapeData m_Vertices (40 bytes), shapes (12 bytes), channels (variable).
    r.counted(40)
    r.counted(12)
    nch = r.i32()
    if not 0 <= nch <= MAX_COUNT:
        raise ReadError('invalid morph channel count')
    for _ in range(nch):
        r.string()
        r.consume(12)
    r.counted(4)  # fullWeights
    r.counted(64) # bind pose matrices
    r.counted(4)  # bone name hashes
    r.u32()       # root bone name hash
    r.counted(24) # BonesAABB min + max
    r.counted(4)  # variable bone count weights
    flags = r.consume(4).tobytes() # meshCompression, isReadable, keepVertices, keepIndices
    index_format = r.i32()
    if index_format not in (0,1):
        raise ReadError(f'index format not supported: {index_format}')
    index_bytes = r.counted_bytes()
    index_width = 2 if index_format == 0 else 4
    if len(index_bytes) % index_width:
        raise ReadError('index buffer misaligned')
    count = len(index_bytes)//index_width
    if count > 5000000:
        raise ReadError('too many indices')
    indices = list(struct.unpack('<'+ ('H' if index_width==2 else 'I')*count, index_bytes))
    vertex_count = r.u32()
    nchannels = r.i32()
    if vertex_count > 2500000 or not (0 <= nchannels <= 32):
        raise ReadError('invalid vertex or channel count')
    channels = []
    for _ in range(nchannels):
        desc = list(r.consume(4))
        desc[3] &= 0x0F  # Unity stores channel dimension in low four bits
        channels.append(tuple(desc))
    vertex_blob = r.counted_bytes()
    for kind in ('float','float','float','float','int','int','int','float','int','int'):
        (_packed_float if kind == 'float' else _packed_int)(r)
    r.u32()  # UVInfo
    r.consume(24) # local mesh bounds
    mesh_usage = r.u32()
    convex_bytes = r.counted_bytes()
    collision_bytes = r.counted_bytes()
    metrics = [r.f32(),r.f32()]
    r.align()
    stream_offset = r.u64()
    stream_size = r.u32()
    stream_path = r.string()
    if r.pos != len(body):
        raise ReadError(f'mesh {name} has {len(body)-r.pos} unexpected trailing bytes')
    if stream_path and (stream_size < 0 or stream_size > 128*1024*1024):
        raise ReadError('stream size invalid')
    if not stream_path and not vertex_blob and vertex_count > 0:
        raise ReadError('vertex data missing despite nonzero vertex count')
    for sub in submeshes:
        if sub['first_byte'] % index_width or sub['first_byte']//index_width+sub['index_count'] > len(indices):
            raise ReadError('submesh indices outside buffer')
    return {'name':name,'index_format':index_format,'indices':indices,
            'submeshes':submeshes,'channels':channels,'vertex_count':vertex_count,
            'vertex_blob':vertex_blob,'stream_offset':stream_offset,
            'stream_size':stream_size,'stream_path':stream_path,
            'flags':list(flags),'mesh_usage':mesh_usage,
            'convex_collision_bytes':len(convex_bytes),
            'triangle_collision_bytes':len(collision_bytes),
            'metrics':metrics,'serialized_size':len(body)}


FORMAT_SIZES = {0:4,1:2,2:1,3:1,4:2,5:2,6:1,7:1,8:2,9:2,10:4,11:4}


def decode_positions(mesh, vertex_bytes):
    """Unity 2018+ channel 0 positional data, with computed 16-aligned streams."""
    n=mesh['vertex_count']
    ch=mesh['channels']
    if n == 0:
        return []
    if not ch or ch[0][3] != 3 or ch[0][2] not in (0,1):
        raise ReadError('unsupported positional channel')
    max_stream=max(c[0] for c in ch if c[3]>0)
    stream_layout={}
    offset=0
    for s in range(max_stream+1):
        stride=sum(c[3]*FORMAT_SIZES[c[2]] for c in ch if c[3]>0 and c[0]==s and c[2] in FORMAT_SIZES)
        if any(c[2] not in FORMAT_SIZES for c in ch if c[0]==s and c[3]>0):
            raise ReadError('unknown vertex attribute format')
        stream_layout[s]=(offset,stride)
        offset=(offset+n*stride+15)&~15
    end_of_last=stream_layout[max_stream][0]+n*stream_layout[max_stream][1]
    if end_of_last>len(vertex_bytes):
        raise ReadError(f'vertex stream too small: expected {end_of_last}, got {len(vertex_bytes)}')
    stream,ofs,fmt,dim=ch[0]
    base,stride=stream_layout[stream]
    positions=[]
    for v in range(n):
        pos=base+stride*v+ofs
        fmt_str='<3f' if fmt==0 else '<3e'
        if pos+FORMAT_SIZES[fmt]*3>len(vertex_bytes):
            raise ReadError('vertex channel outside buffer')
        xyz=struct.unpack_from(fmt_str,vertex_bytes,pos)
        if any(not math.isfinite(x) or abs(x)>1e6 for x in xyz):
            raise ReadError('nonsensical vertex positions')
        positions.append(tuple(float(x) for x in xyz))
    return positions


def triangles(mesh, positions):
    out=[]
    for s in mesh['submeshes']:
        if s['topology']!=0:
            continue
        first=s['first_byte']//(2 if mesh['index_format']==0 else 4)
        if s['index_count']%3:
            raise ReadError('triangle submesh index count not divisible by 3')
        ids=mesh['indices'][first:first+s['index_count']]
        for i in range(0,len(ids),3):
            tri=tuple(int(x)+s['base_vertex'] for x in ids[i:i+3])
            if any(v>=len(positions) for v in tri):
                raise ReadError('triangle outside vertex buffer')
            out.append(tri)
    return out



def encode_ckm1(positions, faces):
    """Experimental big-endian 16-bit vertex/index blob for a *local* N64
    pipeline. CKM1 is NOT a native libdragon/RSP display list.

    Format: 'CKM1' + BE u32 vertexCount/triangleCount + BE center (3 floats)
    + BE float unitsPerInteger, BE i16 XYZ per vertex, BE u16 vertex indices
    per triangle. Caller must avoid uploading game-derived binary to public VCS.
    """
    if not positions or not faces or len(positions) > 65535 or len(faces) > 65535:
        raise ReadError('CKM1 supports 1..65535 vertices and triangles')
    lo=[min(v[d] for v in positions) for d in range(3)]
    hi=[max(v[d] for v in positions) for d in range(3)]
    center=[(a+b)*0.5 for a,b in zip(lo,hi)]
    half=max((b-a)*0.5 for a,b in zip(lo,hi))
    step=max(half/32767.0, 1.0e-10)
    quant=[]
    for p in positions:
        for d in range(3):
            v=int(round((p[d]-center[d])/step))
            if not -32767 <= v <= 32767:
                raise ReadError('CKM1 quantized coordinate overflow')
            quant.append(v)
    indices=[]
    for face in faces:
        if len(face)!=3 or any(not 0 <= v < len(positions) for v in face):
            raise ReadError('CKM1 invalid triangle index')
        indices.extend(face)
    blob=bytearray(b'CKM1')
    blob.extend(struct.pack('>II4f',len(positions),len(faces),*center,step))
    blob.extend(struct.pack('>'+str(len(quant))+'h',*quant))
    blob.extend(struct.pack('>'+str(len(indices))+'H',*indices))
    return bytes(blob)


def decode_ckm1(blob):
    if len(blob)<28 or blob[:4]!=b'CKM1':
        raise ReadError('invalid CKM1 header')
    nv,nt,cx,cy,cz,step=struct.unpack_from('>II4f',blob,4)
    if not 0<nv<=65535 or not 0<nt<=65535 or not math.isfinite(step) or step<=0:
        raise ReadError('invalid CKM1 data')
    if len(blob)!=28+(nv+nt)*6:
        raise ReadError('CKM1 byte length mismatch')
    q=struct.unpack_from('>'+str(3*nv)+'h',blob,28)
    tris=struct.unpack_from('>'+str(3*nt)+'H',blob,28+6*nv)
    if any(i>=nv for i in tris):
        raise ReadError('CKM1 triangle index out of range')
    positions=[(cx+q[3*i]*step,cy+q[3*i+1]*step,cz+q[3*i+2]*step) for i in range(nv)]
    return positions,[tris[3*i:3*i+3] for i in range(nt)]


def scene_external_paths(data):
    """Recover scene fileID -> external file, preserving SerializedFile list order.

    Fails closed on ambiguous/missing references. In this game's v22 metadata
    each entry has a unique zero-terminated basename, with non-resource entries
    matched to the unambiguous installed sharedassetsN file.
    """
    prefix_end=struct.unpack_from('>Q',data,32)[0] # dataOffset
    metadata=data[:prefix_end]
    paths=[]
    for m in re.finditer(rb'(?:globalgamemanagers\.assets|sharedassets\d+\.assets|Library/unity default resources)\x00',metadata):
        name=m.group()[:-1].decode('ascii')
        if name in paths:
            raise ReadError('duplicate external path')
        paths.append(name)
    if not paths or len(paths)>64:
        raise ReadError('could not resolve v22 external file list')
    return {idx+1:path for idx,path in enumerate(paths)}


def inspect_archive(archive, scene_idx=3, selected=None, export_obj=None, export_ckm1=None):
    with zipfile.ZipFile(archive) as z:
        matches={Path(n).name:n for n in z.namelist()}
        managers=matches['globalgamemanagers']
        names=scene_names(z.read(managers))
        if not (0 <= scene_idx < len(names)):
            raise ReadError('invalid scene index')
        scene_bytes=z.read(matches[f'level{scene_idx}'])
        graph=build_hierarchy(parse_serialized(scene_bytes)[0])
        external=scene_external_paths(scene_bytes)
        refs=collections.Counter((m['mesh_file_id'],m['mesh_path_id']) for m in graph['meshes'])
        meshes=[]; all_bad=[]
        cache={}
        for fid,pid in sorted(refs):
            key=f'{fid}:{pid}'
            if selected and key!=selected:
                continue
            external_name=external.get(fid,'')
            if not re.fullmatch(r'sharedassets\d+\.assets',external_name):
                all_bad.append({'mesh_ref':key,'status':'unresolvable_or_builtin',
                                'count':refs[(fid,pid)]})
                continue
            if external_name not in cache:
                raw=z.read(matches[external_name]);cache[external_name]={path:(klass,blob) for path,klass,blob in parse_serialized(raw)[0]}
            target=cache[external_name].get(pid)
            if not target or target[0]!=43:
                all_bad.append({'mesh_ref':key,'status':'not_a_mesh_object',
                                'count':refs[(fid,pid)]})
                continue
            try:
                raw=target[1]
                mesh=parse_mesh(raw)
                data=mesh['vertex_blob']
                stream=mesh['stream_path']
                if stream:
                    stream_name=Path(stream).name
                    if stream_name not in matches:
                        raise ReadError(f'missing external stream: {stream_name}')
                    if mesh['stream_offset']+mesh['stream_size']>z.getinfo(matches[stream_name]).file_size:
                        raise ReadError('stream range outside external resS')
                    # 0.02-0.4 MB streams are normal for small meshes; the ZIP library
                    # reads a single member, never the whole owned archive.
                    buf=z.read(matches[stream_name]); data=buf[mesh['stream_offset']:mesh['stream_offset']+mesh['stream_size']]
                vertices=decode_positions(mesh,data)
                faces=triangles(mesh,vertices)
                if export_obj is not None and key==selected:
                    export_obj.parent.mkdir(parents=True,exist_ok=True)
                    with export_obj.open('w',encoding='utf8') as f:
                        f.write(f'# Private mesh from own game. Do not redistribute.\n')
                        for x,y,zv in vertices:f.write(f'v {x:.8g} {y:.8g} {zv:.8g}\n')
                        for a,b,c in faces:f.write(f'f {a+1} {b+1} {c+1}\n')
                if export_ckm1 is not None and key==selected:
                    export_ckm1.parent.mkdir(parents=True,exist_ok=True)
                    export_ckm1.write_bytes(encode_ckm1(vertices,faces))
                meshes.append({'mesh_ref':key,'name':mesh['name'],'instances':refs[(fid,pid)],
                               'vertices':len(vertices),'triangles':len(faces),
                               'source_file':external_name,'external_stream':bool(stream),
                               'mesh_sha256':hashlib.sha256(raw).hexdigest(),
                               'preview_private_only':bool(export_obj is not None and key==selected)})
            except (ValueError,KeyError,struct.error,OverflowError) as exc:
                all_bad.append({'mesh_ref':key,'status':'unsupported_or_corrupt',
                                'reason':str(exc)[:128], 'count':refs[(fid,pid)]})
        return {'scene':names[scene_idx].split('/')[-1].removesuffix('.unity'),
                'index':scene_idx,'mesh_filter_count':len(graph['meshes']),
                'unique_mesh_references':len(refs),'resolved_meshes':len(meshes),
                'unsupported_refs':len(all_bad),
                'total_triangles_unique_meshes':sum(m['triangles'] for m in meshes),
                'decoded_meshes':meshes,'unresolved':all_bad,
                'notice':'Metadata only; no raw proprietary vertices, textures or game models distributed.'}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('owned_zip',type=Path)
    p.add_argument('--scene',type=int,default=3)
    p.add_argument('--summary',type=Path,required=True)
    p.add_argument('--private-obj',type=Path)
    p.add_argument('--mesh-id',help='Unity fileID:pathID, e.g. 3:179')
    p.add_argument('--private-ckm1',type=Path,help='private experimental BE i16 geometry output')
    args=p.parse_args()
    if (args.private_obj or args.private_ckm1) and not args.mesh_id:
        p.error('--private-obj requires --mesh-id')
    obj=inspect_archive(args.owned_zip,args.scene,args.mesh_id,args.private_obj,args.private_ckm1)
    args.summary.parent.mkdir(parents=True,exist_ok=True)
    args.summary.write_text(json.dumps(obj,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
    print(f"{obj['scene']}: {obj['resolved_meshes']} decoded mesh / "
          f"{obj['unsupported_refs']} unresolved from {obj['unique_mesh_references']} references")
if __name__=='__main__':main()
