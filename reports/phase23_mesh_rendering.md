# Phase 23 — Unity Transform bridge and bounded N64 mesh renderer

**Implementation status: original mesh export and the private libdragon ROM build are verified locally; visual runtime verification in ares remains pending.**

## What Phase 23 adds

- tools/unity_phase23_export.py exports one selected mesh (default tutorial reference 3:194) with its chosen MeshFilter instance's fully resolved parent × local Transform matrix.
- The exporter writes n64/phase23_private_mesh.h for a private local build. It includes source mesh data, transform matrix, and a camera focus/radius computed from transformed vertices. It refuses unresolved/UI-parent transforms and meshes above the limit.
- portable/src/ck64_mesh_scene.c provides checked Unity TRS construction, parent/local composition, point transformation, and budgets.
- n64/mesh_renderer.c clips and projects geometry on the CPU and submits flat-shaded depth-buffered triangles to libdragon's RDP. It samples across the selected mesh and caps RDP submissions per frame.
- C-down toggles the optional preview. A normal public build reports that the private original mesh is missing. A private build includes the generated ignored header; it does not need to publish that header or ROM.

## Verifiable budgets

| Resource | Limit | Estimate |
|---|---:|---:|
| Source CKM1 mesh bytes resident | 128 KiB | Phase 22 sample: 68,590 bytes |
| RDP triangle submissions per frame | 256 | 256 maximum, evenly sampled across the source index range |
| Two 320×240 RGBA16 color buffers | fixed | 307,200 bytes |
| One 320×240 RGBA16 Z buffer | fixed | 153,600 bytes |
| Renderer scratch and view allowance | fixed | 4,096 bytes |
| Renderer-owned RDRAM estimate | maximum | 595,968 bytes |
| Texture TMEM | none | 0 bytes |

The 595,968-byte estimate is buffers + bounded mesh payload + scratch; it excludes executable code, libdragon queues, heap/stack usage, and other existing diagnostic state. Nintendo 64 has 4 MiB RDRAM rather than a separate PC-style VRAM. This untextured first pass uses no texture TMEM.

The reported 4,514 source triangles are not all submitted. The first pass draws at most 256 evenly sampled triangles and has no original materials or textures. That is a geometry pipeline proof, not a complete level.

## Local private export

- Source mesh: TutorialWorld01, reference `3:194`; one resolved MeshFilter placement, 6,913 vertices, 4,514 triangles, 68,590-byte CKM1 output.
- ZIP integrity: 232 entries; Python `zipfile -t` passed, and the archive hash matches the combined ZIP already documented in the README.
- A private libdragon build with the generated original-mesh header completed locally. The resulting private `.z64` is 229,376 bytes, has the N64 magic word, and contains the CKM1 payload marker. The ROM, generated header and preview remain outside the public repository.
- The offline preview was generated from the transformed source geometry. Its N64-camera panel is a software approximation and is **not** a ROM or ares capture.

## Checks and evidence

- The Python suite passed locally: 64 tests (`python -m unittest discover -s tools/tests -v`; 52 Unity tests and 12 ROM diagnostic tests).
- The GitHub portable C11 host test workflow passed on source commit [`4148720`](https://github.com/Dynamon91/Corn-kidz-64-n64-Port-project-/commit/414872065c61662840e3b47216ace72329a088e5), including the synthetic mesh and scene coverage: [run 43626377359](https://github.com/Dynamon91/Corn-kidz-64-n64-Port-project-/actions/runs/43626377359).
- The GitHub public N64 diagnostic build passed on the same source commit: [run 43626377407](https://github.com/Dynamon91/Corn-kidz-64-n64-Port-project-/actions/runs/43626377407). That public build contains no commercial mesh data.
- A separate private libdragon build with the generated TutorialWorld01 mesh header compiled and linked locally. Its ROM is private and is not attached to the public PR.
- The ROM has **not** been visibly loaded and verified in ares. UI interaction was interrupted before the file selection completed, so there is no real ares screenshot and no claim of emulator render success.
- Earlier workflow runs on this branch reported `startup_failure` before starting jobs. The latest CI runs above now start and pass; the historical diagnostic ROM remains unchanged.
- The older Phase 21 diagnostic ROM remains at its existing path. Phase 23 private ROMs must stay local because they contain original game geometry.

## Private reproduction

Use a lawfully obtained original archive locally:

    python3 tools/unity_phase23_export.py /path/to/owned_CornKidz64.zip --scene 3 --mesh-id 3:194 --output n64/phase23_private_mesh.h
    make -C n64 PHASE23_PRIVATE_SCENE=1

The generated header is ignored by Git. This command produced a private ROM locally; C-down enters/exits the preview and the stick orbits the camera. Ares verification remains pending. Keep the ROM local; no emulator image is claimed until it has actually been captured.

## Roadmap

**23 / 50+ milestones = 46% of the provisional plan.** This is milestone progress only, not game completion. The private asset input, mesh export, and private libdragon build are verified. The original-geometry render is not visually verified until the ROM is loaded in ares and a real capture is obtained.
