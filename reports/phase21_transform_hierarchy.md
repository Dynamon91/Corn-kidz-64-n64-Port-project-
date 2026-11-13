# Phase 21 — Original Unity Transform hierarchy and MeshFilter reference audit

**PROJECT IN DEVELOPMENT.** This is verified original scene-graph **metadata**, not a model conversion or playable level. No original mesh vertex buffers, textures, collision meshes, scripts, or audiovisual content are distributed.

## Source and approach

- Private, user-owned Corn Kidz 64 v1.029, Unity 2021.3.30f1 serialized scene files (`level2`, `level3`, `level4`). Their raw data remain outside the repository.
- New `tools/unity_transform_hierarchy.py` reuses the Phase 20 strict SerializedFile v22 reader, extracts local rotation (quaternion), translation and scale from `Transform` class 4, and resolves parent-child matrices `world = parent_world × local_TRS`.
- Detects malformed transforms, invalid quaternions, circular hierarchies, missing parents and references crossing file boundaries.
- `RectTransform` (class 224, UI/layout) is **not** interpreted as a 3D parent. The 3D descendants of such parents retain an unresolved status rather than being assigned fake global coordinates.
- Inspects `MeshFilter` (class 33): its GameObject PPtr and *external* mesh file ID/path ID only. It does **not** decode Mesh geometry or resolve the asset file tables. This cannot yet render the original game environment.
- The scene names are taken from the Unity `globalgamemanagers` Build Settings list, rather than inferred from their file numbers.

## Real scene validation

| Original scene | Native 3D Transforms | Resolved world matrices | Unresolved due to UI parent | MeshFilter references |
| --- | ---: | ---: | ---: | ---: |
| PizzaOut (level2) | 523 | 497 | 26 | 105 |
| TutorialWorld01 (level3) | 1,502 | 1,473 | 29 | 477 |
| Wolloh's Hollow (level4) | 8,458 | 8,432 | 26 | 1,376 |

The reported unresolved counts include descendants of UI transforms; they are not necessarily defective game objects. Counts are from original serialized data and not a polygon budget. The 477 tutorial MeshFilters point to external resource file IDs 2 (79), 3 (222), 4 (101), 6 (75). Actual mesh vertices remain to be decoded from those referenced resources.

## Example: tutorial original anchor transforms

| GameObject name | Resolved Unity world X | Y | Z | Interpretation |
| --- | ---: | ---: | ---: | --- |
| `player` | 19.34088 | 14.18525 | 43.94254 | named source object; *not proof of initial spawn used by all game states* |
| `Main Camera` | 19.34088 | 15.28725 | 47.39554 | parent transform applied |
| `checkpoint` | 36.98877 | 31.69063 | 21.93672 | named source object |
| `spawnPoint` (sample A) | -1.89132 | 2.68576 | 49.50039 | parented: local was (0, 2.12, 0) |
| `spawnPoint` (sample B) | 24.02711 | -30.59961 | 58.48512 | another object sharing the same name |

**Important:** these are world-coordinate metadata for objects in the original scene, *not a reconstructed navigation mesh* or collision geometry. The source scene may contain inactive objects or script-driven/animated transforms; those behaviors have not been interpreted.

## Tests and ROM display

- 16 new synthetic-only Python cases test binary layout, 90° quaternion rotation, parent-scaled translation, multiplication order, UI references, missing/external references, cycles, invalid bodies and external MeshFilter PPtrs. One additional metadata/header consistency check and 8 existing synthetic parser tests also pass: **25 Python tests total**.
- `n64/scene_anchor_meta.h` contains two rounded source positions for a Phase 21 **metadata display in the Z diagnostic menu** when the original scene index is `TutorialWorld01`. This is intentionally *not* a 3D drawing or geometry port; the previously tested wireframe modes and controls are kept unchanged.
- N64 cross-compilation and emulator test must be reported independently. GitHub Actions success proves build validity only, not that it runs in ares.

## Reproduction

```sh
python3 tools/unity_transform_hierarchy.py '/path/to/lawfully-owned-game.zip' --scene 3 --output tutorial_anchors.json
python3 -m unittest discover -s tools/tests -p 'test_unity_transform_hierarchy.py' -v
```

The JSON output contains selected names and spatial metadata only. There is deliberately **no option to export proprietary mesh vertices** or game binaries.

## Next engineering work

1. Resolve external Mesh assets referenced by `MeshFilter` PPtrs using Unity's external references and resources, with explicit provenance and vertex-count limits.
2. Convert a small, verified subset privately into N64-friendly low-poly triangles and indexed vertex buffers. Keep original proprietary game assets out of the public repository unless distribution rights are confirmed.
3. Introduce triangle geometry, depth management, collision primitives, and camera tests in a new isolated N64 mode; compare against original gameplay video and screenshots.
4. Validate any new ROM in ares before making on-device accuracy or performance claims.

**Roadmap:** Phase 21/50+ is about **42% of provisional phase positions**; actual playable-game completion is not quantified.
