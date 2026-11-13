# Phase 12 — CPU wireframe audit on ares

Scope: Instrument the Phase 11 experimental diagnostic environment to investigate
black/gray floor rectangles and ~18-19 VPS in a previous video. **No causal
claim is made yet about graphics corruption or game performance**. The
H.264 recording itself contained decoder warnings.

## New diagnostics
- R (N64 controller shoulder) cycles `GRID:FULL` (18 grid world-lines),
  `GRID:LOW` (10 lines), `GRID:OFF` (0 lines), then back to FULL.
- **Borders, blue pillar, yellow proxy, physics, collision ON/OFF,
  CONTACT!/EJECTED and original GameCtrl diagnostic remain active** in all modes.
- Top-row HUD `L:drawn/attempted` counts segment projection/clip results.
  Expected attempted counts for FULL/LOW/OFF: 46/38/28.
  Drawn lines may be fewer due to world-space near-plane or screen clipping.
- `CPU:NNNNus` shows duration of **PREVIOUS** frame's CPU draw function,
  using libdragon hardware timer. It includes filling the framebuffer,
  software lines, overlay boxes and text, but excludes time in
  `display_get` and `display_show`. CPU duration is **not**
  frames-per-second, emulator VPS, VI pacing, RSP/RDP timing, or real-world
  performance. Count and duration measured with the same scene and camera
  angle make rendering-cost comparisons possible.

## ares test video protocol
1. Load `roms/ck64_diag_phase12_frame_audit.z64` in ares.
2. Stand still facing the blue pillar, with recording ON. Note ares VPS and
   the ROM's CPU duration under `GRID:FULL` for several seconds.
3. Press R once for LOW, once for OFF, once to return FULL. **Do not move
   during this comparison**. Film/photograph mode and CPU numbers.
4. Reproduce the camera orbit (C-left/right) that produced floor patches;
   test FULL and OFF, keeping the same viewpoint.
5. If dark/gray rectangles occur without video capture, note their position
   and ares VPS; send an ares screenshot. If they appear only in recording,
   capture codec is a stronger hypothesis.
6. Test A jump, B reset, START collision, Z diagnostic for regressions.

## Host tests and build
The added `n64/tests/test_wireframe_modes.c` compiles the **actual**
`n64/wireframe.c` on host with stubbed libdragon graphics methods. It
asserts total submitted lines (FULL=46, LOW=38, OFF=28), ensures
all lines handed to the drawing API are within the 320x240 viewport,
checks the diagnostic label, and exercises NULL arguments. Clang/GCC
host test and sanitizers run in GitHub Actions. The N64 workflow
independently recompiles with libdragon and publishes a distinct versioned
`.z64` once successful.

## Scope limitations
This is diagnostic tooling, not Corn Kidz 64's real level rendering or
`PlayerCtrl` port. Any possible FPS improvements from turning the grid
off are local to this prototype. Keep Phase 11 ROM as a baseline.
