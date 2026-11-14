#!/usr/bin/env python3
"""Export one private Unity mesh plus its resolved Transform matrix for Phase 23.

The generated C header contains commercial geometry. Keep it locally and never
commit it. Only public metadata, source code and synthetic fixtures belong in Git.
"""
import argparse
import math
import tempfile
import zipfile
from pathlib import Path

from unity_mesh_probe import decode_ckm1, inspect_archive
from unity_scene_inventory import parse_serialized, scene_names
from unity_transform_hierarchy import build_hierarchy

MAX_PRIVATE_BYTES = 131072


def resolve_instance(records, mesh_id, instance_index=0):
    try:
        file_id, path_id = (int(x) for x in mesh_id.split(":"))
    except (ValueError, AttributeError) as exc:
        raise ValueError("mesh ID must be fileID:pathID") from exc
    graph = build_hierarchy(records)
    matches = []
    for mesh in graph["meshes"]:
        if (mesh["mesh_file_id"], mesh["mesh_path_id"]) != (file_id, path_id):
            continue
        transform_id = graph["go_to_transform"].get(mesh["game_object_id"])
        if transform_id is None or graph["statuses"].get(transform_id) != "world":
            raise ValueError("mesh instance has unresolved or UI-dependent Transform")
        matrix = graph["world_matrices"].get(transform_id)
        if matrix is None:
            raise ValueError("mesh instance has no resolved world matrix")
        matches.append(matrix)
    if not matches:
        raise ValueError("selected mesh has no scene instance")
    if not 0 <= instance_index < len(matches):
        raise ValueError("instance index is outside the selected mesh placements")
    return matches[instance_index], len(matches)


def _cfloat(value):
    if not math.isfinite(value):
        raise ValueError("nonfinite matrix or camera value")
    text = format(float(value), ".9g")
    if "." not in text and "e" not in text.lower():
        text += ".0"
    return text + "f"


def private_header(blob, matrix, source_triangles):
    if len(blob) > MAX_PRIVATE_BYTES:
        raise ValueError("private mesh exceeds 128 KiB N64 resident-data limit")
    positions, faces = decode_ckm1(blob)
    if not positions or not faces:
        raise ValueError("mesh has no renderable triangles")
    world = []
    for x, y, z in positions:
        world.append(tuple(sum(matrix[r][c] * (x, y, z)[c] for c in range(3)) +
                           matrix[r][3] for r in range(3)))
    lo = [min(p[d] for p in world) for d in range(3)]
    hi = [max(p[d] for p in world) for d in range(3)]
    focus = [(lo[d] + hi[d]) * 0.5 for d in range(3)]
    radius = max(math.sqrt(sum((p[d] - focus[d]) ** 2 for d in range(3)))
                 for p in world)
    radius = max(radius, 0.05)
    lines = [
        "#ifndef CK64_PHASE23_PRIVATE_MESH_H",
        "#define CK64_PHASE23_PRIVATE_MESH_H",
        "/* PRIVATE COMMERCIAL GAME DERIVATIVE. DO NOT COMMIT OR DISTRIBUTE. */",
        "#include <stdint.h>",
        '#include "ck64_mesh_scene.h"',
        "static const uint8_t ck64_phase23_mesh_bytes[] = {",
    ]
    for start in range(0, len(blob), 12):
        row = blob[start:start + 12]
        lines.append("    " + ", ".join("0x%02x" % b for b in row) + ",")
    lines.extend([
        "};",
        "static const CK64Affine3x4 ck64_phase23_instance_transform = {{",
    ])
    for row in matrix:
        lines.append("    {" + ", ".join(_cfloat(v) for v in row) + "},")
    lines.extend([
        "}};",
        "static const float ck64_phase23_focus[3] = {" +
        ", ".join(_cfloat(v) for v in focus) + "};",
        "static const float ck64_phase23_radius = " + _cfloat(radius) + ";",
        "#define CK64_PHASE23_PRIVATE_MESH_BYTES sizeof(ck64_phase23_mesh_bytes)",
        "#define CK64_PHASE23_SOURCE_TRIANGLES %du" % source_triangles,
        "#endif",
        "",
    ])
    return "\n".join(lines)


def export(archive, scene, mesh_id, instance_index, output):
    with tempfile.TemporaryDirectory(prefix="ck64-phase23-") as tmp:
        ckm = Path(tmp) / "selected.ckm1"
        summary = inspect_archive(archive, scene, mesh_id, export_ckm1=ckm)
        if summary["resolved_meshes"] != 1 or len(summary["decoded_meshes"]) != 1:
            raise ValueError("selected Unity mesh could not be decoded")
        mesh = summary["decoded_meshes"][0]
        if ckm.stat().st_size > MAX_PRIVATE_BYTES:
            raise ValueError("selected mesh exceeds 128 KiB N64 resident-data limit")
        with zipfile.ZipFile(archive) as z:
            members = {Path(name).name: name for name in z.namelist()}
            scene_names_list = scene_names(z.read(members["globalgamemanagers"]))
            if not 0 <= scene < len(scene_names_list):
                raise ValueError("invalid scene index")
            records = parse_serialized(z.read(members["level" + str(scene)]))[0]
        matrix, instance_count = resolve_instance(records, mesh_id, instance_index)
        ckm1_bytes = ckm.stat().st_size
        blob = ckm.read_bytes()
        header = private_header(blob, matrix, mesh["triangles"])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(header, encoding="utf-8")
    return {"scene": summary["scene"], "mesh_id": mesh_id,
            "vertices": mesh["vertices"], "triangles": mesh["triangles"],
            "instances": instance_count, "instance_index": instance_index,
            "ckm1_bytes": ckm1_bytes, "output": str(output)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("owned_archive", type=Path)
    parser.add_argument("--scene", type=int, default=3)
    parser.add_argument("--mesh-id", default="3:194")
    parser.add_argument("--instance", type=int, default=0)
    parser.add_argument("--output", type=Path,
                        default=Path("n64/phase23_private_mesh.h"))
    args = parser.parse_args()
    result = export(args.owned_archive, args.scene, args.mesh_id,
                    args.instance, args.output)
    print("Private Phase 23 header:", result["output"])
    print("Scene:", result["scene"], "mesh:", result["mesh_id"],
          "vertices:", result["vertices"], "triangles:", result["triangles"],
          "instances:", result["instances"], "selected:", result["instance_index"])
    print("Generated data remains local; do not commit or distribute it.")


if __name__ == "__main__":
    main()
