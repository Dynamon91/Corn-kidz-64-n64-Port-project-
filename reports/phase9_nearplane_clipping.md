# Phase 9 — near-plane clipping repair (experimental wireframe)

**Scope:** Fix the missing lines seen while orbiting the camera in the Phase 8
ares recording. This is not the original Corn Kidz 64 game or its camera system.

## Fault

Previously `n64/wireframe.c::line3d` projected both world-space endpoints
separately and rejected the entire line if either failed. The single-point
projection helper intentionally rejects points behind the 0.35-unit near
plane, so near-plane-crossing grid edges vanished instead of showing their
visible segment.

## Repair

- New portable `ck64_world3d_project_line`: measure both endpoint depths in
  the same world-space camera frame as `ck64_world3d_project`.
- Reject segments completely behind the near plane, and interpolate an
  intersection for segments crossing depth=0.36 (0.01 margin beyond the
  legacy near-plane cutoff) **before** perspective division.
- Preserve 2D Liang-Barsky viewport clipping after perspective.
- Validate input vectors/camera state; keep output points unchanged when
  there is no visible segment or input is invalid.
- `n64/wireframe.c` now uses this helper for the grid, player proxy and
  reference pillar. Rendering remains CPU wireframe using libdragon lines.

## Regression coverage

Two additional host scenario groups (9 world3d groups total):
1. Crossing the near plane in either direction at four camera yaws and
   alternating world boundary positions. A segment with both endpoints
   behind is culled.
2. Wide in-front line clipped to 320x240, NaN, null pointers and unchanged
   outputs on rejected inputs.

Run `cmake -S portable -B build && cmake --build build &&
ctest --test-dir build --output-on-failure`. The existing GitHub Actions
runs GCC, Clang, ASan/UBSan, frontend syntax checks, and the MIPS64
libdragon diagnostic cross-build. When the N64 build succeeds, CI commits
`roms/ck64_diag_phase9_clipping.z64` to main.

## ares comparison test

Keep the previous Phase 8 ROM for A/B comparison. In Phase 9:
1. Start the N64 ROM in ares; verify a wireframe grid and placeholder box.
2. Use C-left/right to orbit near a world edge, then walk toward grid lines.
3. Check whether segments remain visible when passing close to the camera.
4. Test A jump, B reset, Z diagnostics, controller disconnect and recovery.

**Limitations:** This does not address legitimate screen-space clipping,
backface culling, geometry occlusion, original game physics or 3D art.
Emulator success must be established from ares, not merely CI.