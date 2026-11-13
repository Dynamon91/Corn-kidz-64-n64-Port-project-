#!/usr/bin/env python3
"""Inventory owned Corn Kidz 64 Unity 2021.3 scenes, without shipping assets.

Usage:
  python3 tools/unity_scene_inventory.py /path/to/combined.zip --output scene_summary.json
  python3 tools/unity_scene_inventory.py /path/to/combined.zip --private-out private_anchors.json

Input is a personal, lawfully obtained copy of the original game archive. The
script reads Unity SerializedFile v22 *without* embedded type trees. Its public
summary contains counts and selected named anchor data; PRIVATE export adds
all GameObject local positions. It never writes meshes, textures, or audio.
"""
import argparse
import collections
import hashlib
import json
import math
import re
import struct
import zipfile
from pathlib import Path
from typing import Dict, List, Optional, Tuple

SCENE_PREFIX = b"Assets/Scenes/"
# These names refer to semantic labels in the original asset. No mesh data.
ANCHOR_RX = re.compile(r"^(player|checkpoint(?: \(\d+\))?|spawn(?:Point)?|main camera|WORLDX00tut)$", re.I)


def i32(buf: bytes, pos: int) -> int:
    return struct.unpack_from("<i", buf, pos)[0]


def i64(buf: bytes, pos: int) -> int:
    return struct.unpack_from("<q", buf, pos)[0]


def parse_serialized(data: bytes):
    """Return list of (path ID, Unity class ID, binary body) for v22 file.

    Requires version 22 and type tree disabled. Reject malformed metadata,
    indices or out-of-range object bodies rather than guessing structure.
    """
    if len(data) < 64:
        raise ValueError("too short for Unity SerializedFile v22")
    version = struct.unpack_from(">I", data, 8)[0]
    if version != 22:
        raise ValueError("only Unity SerializedFile version 22 is supported")
    metadata_size = struct.unpack_from(">I", data, 20)[0]
    file_size, data_offset = struct.unpack_from(">QQ", data, 24)
    if file_size != len(data) or not (48 < metadata_size < data_offset <= len(data)):
        raise ValueError("invalid size/offset header")
    end = data.find(b"\0", 48, metadata_size)
    if end == -1:
        raise ValueError("missing Unity version string")
    unity_version = data[48:end].decode("ascii")
    o = end + 1
    platform = i32(data, o)
    o += 4
    if data[o] != 0:
        raise ValueError("type-tree-enabled files require a different parser")
    o += 1
    type_count = i32(data, o)
    o += 4
    if not (0 <= type_count <= 8192):
        raise ValueError("implausible serialized type count")
    types: List[int] = []
    for _ in range(type_count):
        if o + 23 > metadata_size:
            raise ValueError("truncated type table")
        klass = i32(data, o)
        o += 7  # class ID, stripped flag and script type index
        if klass == 114:
            o += 16  # MonoBehaviour's script GUID
        o += 16  # type hash
        types.append(klass)
    if o + 4 > metadata_size:
        raise ValueError("missing object count")
    object_count = i32(data, o)
    o = (o + 7) & ~3  # skip object count and align path IDs to 4 bytes
    if not (0 <= object_count <= 1000000) or o + 24*object_count > metadata_size:
        raise ValueError("invalid object table bounds")
    objects = []
    for _ in range(object_count):
        path = i64(data, o)
        start = i64(data, o + 8)
        size, typ = struct.unpack_from("<Ii", data, o+16)
        o += 24
        if not (0 <= typ < len(types)) or start < 0 or size < 0 or start+size > len(data)-data_offset:
            raise ValueError("invalid object record")
        objects.append((path, types[typ], data[data_offset+start:data_offset+start+size]))
    return objects, {"unity_version": unity_version, "platform": platform,
                     "object_count": object_count, "serialized_type_count": type_count}


def game_object(body: bytes) -> Optional[dict]:
    """Unity GameObject: component PPtrs, layer, string m_Name. v22."""
    if len(body) < 13:
        return None
    count = i32(body, 0)
    if not (0 <= count <= 16384):
        return None
    o = 4 + count*12
    if o + 8 > len(body):
        return None
    components = []
    for idx in range(count):
        p = 4 + idx*12
        f = i32(body, p)
        components.append(i64(body, p+4) if f == 0 else None)
    layer = i32(body, o)
    name_size = i32(body, o+4)
    o += 8
    if not (0 <= name_size <= 4096) or o+name_size > len(body):
        return None
    return {"name": body[o:o+name_size].decode("utf-8", "replace"),
            "components": components, "layer": layer}


def transform(body: bytes) -> Optional[dict]:
    """Unity Transform (class 4): m_GameObject PPtr and LOCAL floats."""
    if len(body) < 68 or i32(body, 0) != 0:
        return None
    go_id = i64(body, 4)
    position = struct.unpack_from("<3f", body, 28)
    scale = struct.unpack_from("<3f", body, 40)
    children = i32(body, 52)
    if not (0 <= children <= 100000) or 56+12*children+12 > len(body):
        return None
    if not all(map(math.isfinite, position+scale)):
        return None
    father_offset = 56 + 12*children
    father_id = i64(body, father_offset+4)
    return {"game_object_id": go_id, "local_position": [round(x, 5) for x in position],
            "local_scale": [round(x, 5) for x in scale],
            "parent_transform_id": father_id}


def scan_scene(data: bytes, scene_path: str, include_all: bool = False) -> dict:
    records, meta = parse_serialized(data)
    counts = collections.Counter(klass for _, klass, _ in records)
    gos = {pid:game_object(blob) for pid,klass,blob in records if klass == 1}
    trs = {pid:transform(blob) for pid,klass,blob in records if klass == 4}
    annotations = []
    for go_id, go in gos.items():
        if go is None or not (include_all or ANCHOR_RX.search(go["name"])):
            continue
        local = next((trs[pid] for pid in go["components"] if pid in trs and trs[pid]),None)
        if not local:
            continue  # UI RectTransforms intentionally excluded
        annotations.append({"name":go["name"], "local_position":local["local_position"],
                            "has_parent": local["parent_transform_id"] != 0})
    annotations.sort(key=lambda v:(v["name"].lower(), v["local_position"]))
    return {"scene":scene_path, "sha256":hashlib.sha256(data).hexdigest(),
            **meta, "class_counts":{"GameObject":counts[1],"Transform":counts[4],
                                   "MonoBehaviour":counts[114],"MeshFilter":counts[33],
                                   "MeshRenderer":counts[23],"Camera":counts[20]},
            "anchors":annotations}


def scene_names(global_manager: bytes) -> List[str]:
    """Read length-prefixed build-settings path strings in serialized data.

    Offsets are not used to infer a scene if path evidence is missing.
    """
    matches = []
    for match in re.finditer(rb"Assets/Scenes/[a-zA-Z0-9 '\-_.]+\.unity", global_manager):
        matches.append(match.group().decode("ascii"))
    return list(dict.fromkeys(matches))


def inspect_archive(archive: Path, include_all: bool = False) -> dict:
    scenes = []
    with zipfile.ZipFile(archive) as z:
        name_map = {Path(n).name:n for n in z.namelist() if re.fullmatch(r"level\d+",Path(n).name)}
        managers = next((n for n in z.namelist() if n.endswith("/globalgamemanagers")),None)
        if managers is None:
            raise ValueError("missing Unity globalgamemanagers")
        paths=scene_names(z.read(managers))
        for i in range(len(paths)):
            filename=f"level{i}"
            if filename not in name_map:
                raise ValueError(f"scene listed in build settings missing: {filename}")
            s=scan_scene(z.read(name_map[filename]),paths[i],include_all)
            s["index"]=i
            scenes.append(s)
    return {"source":"user-owned original Corn Kidz 64 Unity 2021.3.30f1",
            "parser_scope":"SerializedFile v22, no embedded type tree, class 1 and 4 only",
            "position_semantics":"LOCAL Unity Transform position; not world coordinates",
            "scenes":scenes}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("archive",type=Path,help="private combined original ZIP")
    ap.add_argument("--output",type=Path,help="save metadata-only public summary")
    ap.add_argument("--private-out",type=Path,help="save detailed GameObject locations locally ONLY")
    args=ap.parse_args()
    if not args.output and not args.private_out:
        ap.error("specify --output or --private-out")
    if args.output:
        result=inspect_archive(args.archive)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf8")
        print(f"Indexed {len(result['scenes'])} Unity scenes -> {args.output}")
    if args.private_out:
        result=inspect_archive(args.archive,include_all=True)
        args.private_out.parent.mkdir(parents=True,exist_ok=True)
        args.private_out.write_text(json.dumps(result,indent=2)+"\n",encoding="utf8")
        print(f"PRIVATE object index -> {args.private_out} (do NOT commit)")

if __name__=="__main__":
    main()