# Phase 20 — Original Unity scene/build index recovered

**Project in development. This is metadata recovery, not a playable Corn Kidz 64 level.**

## Evidence and method

- Input: user-provided original Corn Kidz 64 v1.029 Windows Unity 2021.3.30f1, privately reconstructed ZIP. Original proprietary ZIP, texture/model/audio payloads and full scene object data are **not** placed in GitHub.
- Parsed the v22 Unity SerializedFile headers, little-endian type/object tables, `GameObject` names, `Transform` local positions and object reference links. Validated all 18 scene files and the build settings ordering in `globalgamemanagers`.
- Each `levelN` corresponds to Build Settings index `N`, based on the observed ordered Unity `Assets/Scenes/*.unity` path entries and scene objects. This is a source-verified mapping, not a guess based on video timestamps.
- Important: Unity **serialized object counts are not polygon counts**. `MeshFilter` counts are not independently usable models. Local Transform positions must **not** be confused with coordinates in world space (parent chains are not yet resolved).
- The tool `tools/unity_scene_inventory.py` uses only the Python standard library and never runs Unity executables or publishes a mesh or texture.

## Build-settings scene catalog

| Build index | Original scene | Serialized objects | GameObjects | MeshFilters | MonoBehaviours |
| ---: | --- | ---: | ---: | ---: | ---: |
| 0 | `launcher` | 268 | 69 | 2 | 60 |
| 1 | `title01` | 1,153 | 322 | 49 | 212 |
| 2 | `PizzaOut` | 2,055 | 618 | 105 | 319 |
| 3 | `TutorialWorld01` | 6,000 | 1,597 | 477 | 986 |
| 4 | `Wolloh's Hollow` | 25,690 | 8,553 | 1,376 | 2,853 |
| 5 | `Tower00` | 7,864 | 2,685 | 395 | 822 |
| 6 | `bonus01` | 1,393 | 379 | 76 | 257 |
| 7 | `bonus02` | 1,201 | 336 | 54 | 215 |
| 8 | `gameover` | 1,165 | 326 | 51 | 212 |
| 9 | `credits` | 3,302 | 1,312 | 54 | 229 |
| 10 | `bonus03` | 1,358 | 364 | 90 | 231 |
| 11 | `bonus04` | 1,370 | 374 | 77 | 244 |
| 12 | `bonus05` | 1,230 | 342 | 61 | 206 |
| 13 | `TowerN00` | 7,313 | 2,341 | 442 | 851 |
| 14 | `TestZoneX` | 3,825 | 1,004 | 328 | 528 |
| 15 | `secretZone00` | 2,606 | 660 | 196 | 466 |
| 16 | `results` | 1,206 | 337 | 49 | 223 |
| 17 | `bonus06` | 1,431 | 392 | 76 | 248 |

## First playable-area investigation

- Build index 2: `PizzaOut` (2,055 objects), local `player` transform `(-7.20436, 2.04541, 11.88406)`; local `checkpoint` `(-10.15, 1.94542, -9.55)`.
- Build index 3: `TutorialWorld01` (6,000 objects), local `player` transform `(19.34088, 14.18525, 43.94254)` and explicit world-label object `WORLDX00tut`; its local `checkpoint` is **parented**, so its `m_LocalPosition` alone is NOT a global location.
- Build index 4: `Wolloh's Hollow` (25,690 objects) has multiple checkpoint labels. Do not interpret raw named checkpoint transforms as collision/spawn points before resolving hierarchy and scripts.
- Original gameplay reference: https://www.youtube.com/watch?v=nIsAuFddStQ — navigation footage helps validate later reconstructed geometry and game flow.

## Verification and limitations

8 Python stdlib test cases cover tiny synthetic SerializedFile fixtures, structure validation, malformed input rejection, local Transform semantics and named anchors. In the analysis workspace the parser also successfully scanned **all 18 real scenes** without an object-table bounds error.

The renderer is still a libdragon wireframe test room. **No original level mesh or character model has been imported**, no complete Unity scripts have been translated, and no commercial game footage is embedded. Scene inventory cannot substitute for actual mesh/collision extraction.

## Reproduce privately with legally obtained original files

```bash
python3 tools/unity_scene_inventory.py '/path/to/CornKidz64_combined.zip' --output scene_summary.json
python3 -m unittest discover -s tools/tests -p 'test_unity_scene_inventory.py' -v
```

Optional `--private-out my_unity_objects.json` includes a broader list of *local* object positions and should be retained privately, not uploaded to public GitHub.

## N64 Phase 20 diagnostic

In the N64 diagnostic screen (Z), R cycles the 18 scene catalog entries and shows game-object/mesh-filter counts from the recovered build index. This UI is a verification tool only: none of the scene assets are loaded by the ROM.

**Provisional roadmap:** 20 / 50+ milestone positions (~40% of a phase-counting plan), not 40% completion of authentic Corn Kidz 64 gameplay.

Next: resolve `Transform` parent chains, assess MeshFilter mesh references and bounds, and build the first reduced-geometry test map from the owner's own game data, with explicit per-asset provenance. Do not upload proprietary raw content to this public repository.