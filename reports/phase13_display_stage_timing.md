# Phase 13 — Separate display_get/draw/display_show timing (ares diagnostic)

This diagnostic ROM remains a wireframe test: **no original Corn Kidz 64
models, stage geometry, audio, collision mesh or authentic PlayerCtrl yet**.

## What is measured
With libdragon timer ticks, the 3D frontend now snapshots:
- **G**: time from calling `display_get` until it returns (may include waiting
  for an available framebuffer in a double-buffered display).
- **D**: time inside the CPU wireframe drawing function, including its HUD.
- **S**: time spent inside `display_show` before it returns.
- **T**: total from just before `display_get` to immediately after
  `display_show` returns. This covers G+D+S, but **not** game logic,
  controller polling, real VI flip/display scanout or emulated host scheduling.

All four values are microseconds (us), saturated at 99999 for HUD safety,
and refer to the **previous completed display frame**. Invalid monotonic tick
sequences cause `G/D/S/T: NO TIMING SAMPLE` rather than fake zeros.

`GRID:FULL` / `LOW` / `OFF` remain selectable using controller R, and
line counts `L:drawn/attempted` remain in the top HUD. The CONTACT!/EJECTED
overlay has been moved down to avoid covering the new timing rows.

## How to test on ares (record short 20–30-second video)
1. Open `roms/ck64_diag_phase13_display_timing.z64` on ares.
2. Keep the player/camera still with GRID FULL. Record G, D, S, T and ares VPS.
3. Press R once for LOW and once for OFF, staying at the same camera angle.
4. If VPS drops, note which of G/D/S/T changes. Also note that VPS can drop
   for reasons *outside* these timed sections.
5. Turn the camera with C-left/right, then try FULL and OFF.
6. Check A jump, START collision ON/OFF, B reset and Z diagnostics.
7. If ares shows gray floor blocks, compare with a native screenshot
   taken without video capture.

## Tests/limitations
`portable/tests/test_perf.c` checks exactly separated tick durations, zero
length, clamping and atomic rejection of invalid timestamps. The existing
host wireframe test checks the actual HUD output plus its unchanged line
budgets. GitHub CI compiles with GCC, Clang and sanitizers, then generates
a native N64 ROM. A successful compile is not ares runtime validation.

These timings are a diagnostic for the CPU software wireframe prototype,
not a benchmark of libdragon 3D RSP/RDP performance. Neither ares VPS nor
hardware timing may be inferred directly from one displayed value.
