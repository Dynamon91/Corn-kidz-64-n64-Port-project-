# Phase 19 — Source-driven experimental movement room

> **PROJECT IN DEVELOPMENT.** This ROM is a diagnostic-only N64 homebrew, **not** a port of authentic Corn Kidz 64 levels, characters, textures, sound, animation or complete physics.

## New integration

A new self-contained `CK64SourceLab` instance runs the Phase 14–18 source-grounded reconstructed parts of `PlayerCtrl`: `ck64_player_state0_step` selects movement/jump rules, `ck64_player_motion_step` computes horizontal movement and rotation, and `ck64_player_run_footer_step` updates grounded/airborne history and derived landing edges. This state now **moves the visible wireframe avatar** in a separate test scene. Before Phase 19, the source logic was only probed on-screen and the visible avatar was entirely controlled by placeholder `ck64_world3d_step`.

The *source-grounded* elements are the selected `Gravity` constants, `MovPl` operation ordering, state-0 jump thresholds, early-release short hop, and floor-timer rules. They are partial reconstructions from original v1.029 IL and **not numerically verified against original Unity gameplay**.

## Explicitly synthetic adapter

- Camera-relative analog mapping, joystick dead zone and camera orbit are experimental.
- A flat y=0 floor, 15x15 bound and one AABB pillar are test geometry, **not original game collision maps**. Pillar collision toggles with START.
- Y displacement applies the source's yVel **once per 30 Hz experimental diagnostic update**, not a validated Unity fixed update cadence. X/Z displacement uses `MovPl`'s `plVel` once per test update. Unit scales and timestamps are provisional.
- `add_vel_magnitude=0`, no headbutt/climb/other PlayerCtrl states, no animation events or original assets.
- The new adapter uses a fresh state separate from `CK64World3D`'s legacy diagnostics, so the previous user-verified diagnostics are preserved.

## Controls (wireframe screen)

- **L** cycles `LEGACY` → `SOURCE PROBE` (Phase 18) → **`SOURCE LIVE`** → `LEGACY`. `SOURCE LIVE` starts from a new spawn position and independent counters.
- In LIVE: stick moves the avatar, **A** jumps (releasing A early gives the short hop), **B** resets the live lab, **C-Left/Right** orbits the experimental camera, **START** toggles the temporary pillar collider, **R** cycles FULL/LOW/OFF grid, and **Z** toggles the diagnostic gameflow screen. No original game input mapping is claimed.
- On-screen LIVE HUD displays movement speed parameter `zMov`, original yVel, floor history, counts of jumps and landings, and source movement-path selection.

## Testing and limitations

Ten C11 scenario groups cover initialization, idle/floor history, actual X/Z motion, friction, jump and landing, early release, camera-relative input, pillar blocking, reset, and atomic invalid-input rejection. Locally: GCC/Clang -Werror and Clang ASan/UBSan pass against version-compatible local source files with a host-only test world initialization stub. CI performs complete linkage and N64 cross-compilation; **no ares validation has occurred yet**.

The user-supplied original C# game assembly and 7-hour YouTube footage are research inputs; they are **not** included in the repo or ROM. In particular, this is **not a Corn Kidz level recreation**.

Progress convention: 19/50 = **38% of a provisional milestone checklist**, not 38% of game conversion progress. Full original-game completeness is not measurable yet.
