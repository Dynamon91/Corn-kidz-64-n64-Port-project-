# Phase 18 — PlayerCtrl.Run floor history and landing-edge diagnostics

**PROJECT IN DEVELOPMENT — not a playable original Corn Kidz 64 release.**

Source: privately inspected user-owned Corn Kidz 64 v1.029, `PlayerCtrl::Run` token `0x06000294` at IL offsets **75849–76018**. Source bodies and proprietary assemblies are not committed.

## Source-derived rules
1. `plTimer`, `plTimer2`, `plTimer3` increment by 1 per Run update.
2. `bNoAir` resets to false.
3. `onFloorTimer` uses `max(0, old)+1` when grounded; `min(0, old)-1` in the air. This yields -1 immediately after leaving ground, then -2/-3/-4. Phase 17 permits the jump while the timer is greater than -4.
4. `lastWallObj` clears on floor or in state 4, 11 or 12.

The C11 implementation stores only a boolean wall-object presence flag and *derived* edge markers. Overflow guards are C11 safety additions. Original Unity physics, Blink, lastPos, bDeparent, collision detection, assets and animations are still missing.

## Tests
11 scenario groups: grounded, airborne, floor exit, landing, coyote boundary, grounded wall clear, special-state wall clear, counters/noAir, atomic rejection, false landing suppression, and integrated Phase 17 jump with early release. Local GCC, Clang and sanitizers passed. CI independently validates on GitHub.

## N64 experimental probe
Use **L** in wireframe diagnostic to toggle source floor/JUMP probe. A samples the source-derived jump using the independent floor history, which in turn reads the *prototype* CK64World3D grounded flag once per fixed update. FloorT, source sample yVel and derived landing count are displayed. **These values do NOT yet move the visible character or reproduce original Unity collision.** Ares boot/overlay is not verified until an emulator video is supplied.

## Roadmap
18/50 = **36% of an illustrative milestone roadmap**, not 36% of the commercial game rebuilt. Next: isolated source-driven movement playground, actual landing/raycast and animation evidence.
