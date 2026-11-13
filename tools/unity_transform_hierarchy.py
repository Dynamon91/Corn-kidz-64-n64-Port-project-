#!/usr/bin/env python3
"""Resolve v22 Unity Transform TRS chains from a lawfully owned PC game.

This is a metadata-only diagnostic. It does not decode Mesh vertices, animation,
colliders, or RectTransform/UI spatial semantics. No proprietary input is stored
in the public repository.  All cross-file object references stay unresolved.

Usage:
  python3 tools/unity_transform_hierarchy.py own_game.zip --scene 3 --output private.json
  python3 tools/unity_transform_hierarchy.py own_game.zip --scene 3 --public reports/phase21_tutorial_anchors.json
"""
import argparse
import collections
import json
import math
import struct
import zipfile
from pathlib import Path
from typing import Dict
from unity_scene_inventory import parse_serialized, game_object, scene_names


def pptr(body, offset):
    if offset + 12 > len(body):
        raise ValueError('truncated Unity PPtr')
    return struct.unpack_from('<iq', body, offset)


def read_transform(body):
    if len(body) < 68:
        raise ValueError('truncated Transform')
    file_id, go_id = pptr(body, 0)
    if file_id != 0:
        raise ValueError('external GameObject reference')
    rotation = struct.unpack_from('<4f', body, 12)
    position = struct.unpack_from('<3f', body, 28)
    scale = struct.unpack_from('<3f', body, 40)
    count = struct.unpack_from('<i', body, 52)[0]
    if count < 0 or count > 100000 or 68 + count*12 > len(body):
        raise ValueError('invalid children count')
    children = [pptr(body, 56 + i*12) for i in range(count)]
    parent = pptr(body, 56 + count*12)
    if not all(math.isfinite(v) for v in (*rotation, *position, *scale)):
        raise ValueError('nonfinite transform')
    if sum(v*v for v in rotation) < 1e-12:
        raise ValueError('zero quaternion')
    return {'game_object_id': go_id, 'rotation': rotation,
            'position': position, 'scale': scale, 'children': children,
            'parent': parent}


def read_mesh_filter(body):
    if len(body) != 24:
        raise ValueError('unexpected MeshFilter layout')
    go_file, go = pptr(body, 0)
    mesh_file, mesh = pptr(body, 12)
    if go_file != 0:
        raise ValueError('MeshFilter externally owned GameObject')
    return {'game_object_id': go, 'mesh_file_id': mesh_file,
            'mesh_path_id': mesh}


def local_matrix(tr):
    x, y, z, w = tr['rotation']
    norm = math.sqrt(x*x + y*y + z*z + w*w)
    x, y, z, w = x/norm, y/norm, z/norm, w/norm
    sx, sy, sz = tr['scale']; px, py, pz = tr['position']
    xx, yy, zz = x*x, y*y, z*z
    xy, xz, yz = x*y, x*z, y*z
    wx, wy, wz = w*x, w*y, w*z
    # Rows of a 3x4 affine matrix: R(quaternion) * S(local-scale), plus T.
    return (( (1-2*(yy+zz))*sx, 2*(xy-wz)*sy, 2*(xz+wy)*sz, px),
            ( 2*(xy+wz)*sx, (1-2*(xx+zz))*sy, 2*(yz-wx)*sz, py),
            ( 2*(xz-wy)*sx, 2*(yz+wx)*sy, (1-2*(xx+yy))*sz, pz))


def product(a, b):
    return tuple(tuple(sum(a[i][k]*b[k][j] for k in range(3))+
                       (a[i][3] if j == 3 else 0.0)
                       for j in range(4)) for i in range(3))


def build_hierarchy(records):
    classes = {pid: klass for pid, klass, _ in records}
    trs = {pid: read_transform(blob) for pid,klass,blob in records if klass == 4}
    names = {pid: go['name'] for pid,klass,blob in records if klass == 1
             if (go := game_object(blob)) is not None}
    meshes = [read_mesh_filter(blob) for pid,klass,blob in records if klass == 33]
    memo: Dict[int, tuple] = {}
    status: Dict[int,str] = {}
    state = {}
    def resolve(pid):
        if pid in memo:
            return memo[pid]
        if state.get(pid) == 1:
            raise ValueError(f'Transform parent cycle at {pid}')
        state[pid] = 1
        tr = trs[pid]
        file_id, parent_id = tr['parent']
        if file_id != 0:
            parent_status, parent_mat = 'external_parent', None
        elif parent_id == 0:
            parent_status, parent_mat = 'world', None
        elif parent_id not in trs:
            parent_status = ('unsupported_rect_transform' if classes.get(parent_id) == 224
                             else 'unresolved_parent')
            parent_mat = None
        else:
            parent_mat = resolve(parent_id)
            parent_status = status[parent_id]
        status[pid] = parent_status
        matrix = local_matrix(tr) if parent_status == 'world' else None
        if matrix is not None and parent_mat is not None:
            matrix = product(parent_mat, matrix)
        memo[pid] = matrix
        state[pid] = 2
        return matrix
    for pid in trs:
        resolve(pid)
    go_to_transform = {tr['game_object_id']:pid for pid,tr in trs.items()}
    return {'transforms': trs, 'names':names, 'meshes':meshes,
            'world_matrices':memo, 'statuses':status, 'go_to_transform':go_to_transform}


def summarize_scene(records, scene, index, max_anchors=32):
    graph = build_hierarchy(records)
    status = collections.Counter(graph['statuses'].values())
    meshfiles = collections.Counter(str(m['mesh_file_id']) for m in graph['meshes'])
    # These anchors are deliberately a small metadata sample, not a level map.
    named = []
    for go_id, name in graph['names'].items():
        if name.lower() not in ('player', 'checkpoint', 'spawn', 'spawnpoint',
                                'main camera', 'worldx00tut'):
            continue
        pid = graph['go_to_transform'].get(go_id)
        if pid is None:
            continue
        mat = graph['world_matrices'][pid]
        named.append({'name':name, 'world_position':
                      [round(mat[i][3], 5) for i in range(3)] if mat else None,
                      'resolution':graph['statuses'][pid]})
    named.sort(key=lambda o:(o['name'].lower(), str(o['world_position'])))
    return {'scene':scene,'build_index':index,
            'native_transforms':len(graph['transforms']),
            'transform_status_counts':dict(sorted(status.items())),
            'mesh_filters':len(graph['meshes']),
            'mesh_reference_file_id_counts':dict(sorted(meshfiles.items())),
            'geometry_extracted':False,
            'anchors':named[:max_anchors],
            'anchor_count':len(named)}


def inspect(archive, index=3):
    with zipfile.ZipFile(archive) as z:
        matches=[n for n in z.namelist() if n.endswith('/level'+str(index))]
        if len(matches)!=1:
            raise ValueError('expected exactly one Unity scene file')
        managers = [n for n in z.namelist() if n.endswith('/globalgamemanagers')]
        if len(managers) != 1:
            raise ValueError('expected exactly one Unity build-settings manager')
        ordered_names = scene_names(z.read(managers[0]))
        if not (0 <= index < len(ordered_names)):
            raise ValueError('scene index not present in Build Settings')
        rec,meta=parse_serialized(z.read(matches[0]))
    summary=summarize_scene(rec,ordered_names[index].split('/')[-1].removesuffix('.unity'),index)
    summary['unity_version']=meta['unity_version']
    return summary


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('original_archive',type=Path)
    ap.add_argument('--scene',type=int,default=3)
    ap.add_argument('--output', '--public',dest='output',required=True,type=Path)
    args=ap.parse_args()
    result=inspect(args.original_archive,args.scene)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf8')
    print(f"Resolved {result['native_transforms']} native Transforms; "
          f"{result['transform_status_counts']}; wrote {args.output}")
if __name__ == '__main__':
    main()
