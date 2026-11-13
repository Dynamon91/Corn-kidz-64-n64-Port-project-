# Phase 22 — Original geometry extraction and experimental CKM1 N64 data path

**PROJECT IN DEVELOPMENT; no playable Corn Kidz 64 N64 release yet.** The latest user-validated visual ares execution remains Phase 13; new N64 builds still need emulator confirmation.

## Source and provenance

Read the **user-provided original Corn Kidz 64 v1.029** Unity 2021.3.30f1 archive privately (not checked into GitHub). The new `tools/unity_mesh_probe.py` uses the verified v22 `SerializedFile` and `Transform` readers from Phases 20–21, resolves ordered scene fileIDs to external sharedassets files, parses Mesh class 43 submeshes, 16/32-bit index buffers, position channels, optional `.assets.resS` vertex streams, and triangle index ranges. It rejects unknown layouts and never silently fabricates missing geometry. The archive's `Library/unity default resources` references are marked **built-in/unresolved** rather than guessed. Parser layout corroborated with [AssetStudio's publicly documented Mesh reader](https://github.com/Perfare/AssetStudio/blob/master/AssetStudio/Classes/Mesh.cs) (independently implemented C/Python source in this project).

## Verified measurements on owner's original game data

| Scene | MeshFilter objects | Unique referenced meshes | Independently decoded | Built-in refs | Triangles in unique decoded meshes |
|---|---:|---:|---:|---:|---:|
| TutorialWorld01 | 477 | 116 | 113 | 3 | 8,605 |
| Wolloh's Hollow | 1,376 | 319 | 315 | 4 | 26,407 |

These triangle totals are per *unique resource*, **not per placement instance or final render frame**. Terrain, animated/skinned pipeline and original material/texture presentation remain unported.

A 3D mesh genuinely present in tutorial is `000world` (scene external ref `3:194`): **6,913 raw position vertices / 4,514 triangles**. Private extraction successfully produced indexed OBJ geometry and a compact big-endian CKM1 blob without creating/inventing a replacement mesh. The original mesh bounding box and visual offline wireframe were used to sanity-check coordinate plausibility. The preview in the conversation is an **offline 3D wireframe of the extracted original model**, NOT a screenshot of the libdragon renderer.

## N64-ready experimental exchange format

`CKM1` v0, authored for this project:

- BE 4-byte ASCII magic `CKM1`;
- BE uint32 vertex count, uint32 triangle count;
- BE float32 local center ×3 and quantization step ×1;
- BE signed int16 ×3 for each vertex, and unsigned uint16 ×3 for each triangle.

The private `000world.ckm1` is **68,590 bytes** (versus 6,913×12 bytes for float XYZ plus 4,514×6 for indices, ~110 KiB). Uniform 16-bit quantization max Euclidean reconstruction error **0.00271875 Unity units**, RMS **0.00163176 Unity units**. Only CPU decoding is implemented in `portable/src/ck64_mesh_format.c`. This is **not** a hardware-compatible RSP/RDP display list, and it does not load a proprietary mesh into any public ROM yet.

## Verification and privacy

- Python `test_unity_mesh_probe.py`: **21 synthetic-only unittest cases**, no shipped commercial game binary. Tests cover format header, index types, vertices, external streams, malformed data, dimension masks, CKM1 reconstruction and bounds.
- C11 `test_mesh_format.c`: **10 scenario groups** with GCC/Clang and sanitizer compatibility checks.
- Local source-data verification: exact referred mesh object reached via external Unity file IDs; 113/116 and 315/319 decoded. Four or three unavailable entries are **Unity built-in primitive refs**, *not* decoder failures.
- Store source code, this metadata-only report, and tests on GitHub. **Do not commit original raw `.obj`, `.ckm1`, `.resS`, original Unity assets, or footage to the public repository**. The owner may privately generate them with their lawfully obtained game copy.
- Keep the Phase 21/19 diagnostic ROM unmodified: integration of graphics rendering must be benchmarked and tested separately.

## Reproduce with a personally owned archive

```sh
python3 tools/unity_mesh_probe.py /path/to/owned_CornKidz64.zip --scene 3 --summary /private/tutorial_mesh_summary.json
python3 tools/unity_mesh_probe.py /path/to/owned_CornKidz64.zip --scene 3 --mesh-id 3:194 \
  --summary /private/selected_mesh.json --private-obj /private/000world.obj --private-ckm1 /private/000world.ckm1
python3 -m unittest discover -s tools/tests -p test_unity_mesh_probe.py -v
cmake -S portable -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

**Progress convention:** Phase 22/50 = 44% of a *provisional roadmap*, **not 44% of the original game recreated**.

**Next:** map geometry instances to Transform world matrices, introduce an N64 renderer/batching subsystem with triangle budgeting and clipping, then benchmark on ares before presenting a visual gameplay milestone. No texture fidelity is claimed.
