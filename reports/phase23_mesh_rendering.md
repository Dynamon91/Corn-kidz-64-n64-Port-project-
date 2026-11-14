# Phase 23 — Unity Transform bridge and bounded N64 mesh renderer

**Implementation status: renderer path, exporter, and synthetic coverage added; original N64 render remains unverified.** The owner supplied the original archive as three split files. They were reassembled only in the private local workspace; the reconstructed ZIP matches the SHA-256 already recorded in the repository, and all 232 entries pass ZIP CRC checks. The exporter successfully resolved the TutorialWorld01 `3:194` MeshFilter placement and generated a local CKM1 header: 6,913 vertices, 4,514 triangles, 68,590 bytes. The header and offline preview stay outside GitHub. No commercial bytes were added to this public repository.

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
- An offline preview was generated from the transformed source geometry. Its N64-camera panel is a software approximation and is **not** a ROM or ares capture. The private generated header and PNG remain outside the public repository.

## Checks and evidence

- C11 synthetic tests cover quaternion/scale/translation, parent × local order, invalid-input atomicity, triangle caps, 128 KiB rejection, and memory arithmetic.
- Python synthetic tests cover Transform-to-MeshFilter association, inherited parent scale, unresolved ancestry rejection, mixed resolved/unresolved placements, and the private generated-header marker.
- A local run against public branch sources at `f0913a66b209a330122532c451a14546042a1adf` passed all 64 Python tests: `python -m unittest discover -s tools/tests -v` (52 Unity tests plus 12 ROM diagnostic tests).
- The current desktop has no CMake, GCC, Clang, MIPS64 GCC, or Make on PATH, so it could not run a local C11 or libdragon build.
- At commit `2c96575b929ee5428dc24cfbcd916f94cc0712ed`, the GitHub N64 diagnostic build passed ([run](https://github.com/Dynamon91/Corn-kidz-64-n64-Port-project-/actions/runs/43563119291)); GCC and Clang host jobs passed, as did Python tests and the 15 CTest cases.
- The sanitizer job's final N64 frontend syntax step failed at that commit because four libdragon stub headers were missing. They were added in commit `018c31486087eb89f4ac1867eff8facb8d4c057b`.
- Runs for `018c314`, `b3fbf62`, `3b27bec`, `cc62d7c`, and the source-test head `f0913a6` ended in `startup_failure` before creating jobs (see [f0913a6 run](https://github.com/Dynamon91/Corn-kidz-64-n64-Port-project-/actions/runs/43565834948)). Therefore, the stub fix and mixed-placement regression test are **not yet verified by CI**. No private-header N64 build or ares visual test has run.
- The older Phase 21 diagnostic ROM remains at its existing path. Phase 23 private ROMs must stay local because they contain original game geometry.

## Private reproduction

Use a lawfully obtained original archive locally:

    python3 tools/unity_phase23_export.py /path/to/owned_CornKidz64.zip --scene 3 --mesh-id 3:194 --output n64/phase23_private_mesh.h
    make -C n64 PHASE23_PRIVATE_SCENE=1

The generated header is ignored by Git. In the ROM, C-down enters/exits the preview, and the stick orbits the camera. Press Z to return to metadata diagnostics. Keep the ROM local and verify it in ares before reporting any visual result.

## Roadmap

**23 / 50+ milestones = 46% of the provisional plan.** This is milestone progress only, not game completion. The private asset input and local mesh export are now verified. Phase 23 still requires a successful private ROM build and an actual ares visual capture before an original-geometry N64 render can be called verified.
