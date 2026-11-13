# Phase 8 — Nintendo 64 wireframe navigation prototype

## Scope and provenance

This phase adds a *diagnostic perspective 3D wireframe demo* to the already tested
libdragon ROM; no assets from Corn Kidz 64 are reproduced or included, and the
movement logic is **not** asserted to match original Unity `PlayerCtrl`.
Original game player motion, authored collision geometry, animations, camera
zones, and 3D character model remain unported.

The world consists of a flat square grid, one landmark wireframe pillar and
an avatar bounding box. Uses libdragon `graphics_draw_line`, CPU-side 3D
projection and clipping rather than a hardware-accelerated 3D pipeline; the
purpose is validating input, update scheduling and camera before implementing
RDP/RSP rendering.

## Controls in ares

- **Analog stick**: move the yellow placeholder relative to the camera.
- **A**: jump; B: reset world position and camera.
- **C-Left / C-Right**: orbit camera around avatar.
- **Z**: switch to the old diagnostic status screen. In diagnostic mode,
  A/B still test scene transitions and Start still toggles the music flag.

The wireframe view launches by default after BOOT 1/2/3.
Only run the ROM with a working Controller 1 mapping.

## Architecture

- `portable/include/ck64_world3d.h`: public structs and API.
- `portable/src/ck64_world3d.c`: deterministic 30 Hz planar movement,
  one-stage jump/gravity, square world boundary clamp, camera-relative move,
  perspective project and 2D viewport clipping.
- `portable/tests/test_world3d.c`: seven host scenarios including projection
  clipping, grounded state, camera steer, bounded movement and input validation.
- `n64/wireframe.c`: 3D line segments for grid, pillar and player bounding box.
- `n64/main.c`: libdragon input bridge and 3D/diagnostic mode switching.
- `n64/tests/stubs/libdragon.h`: updated host syntax validation declarations.

## Development checks

```sh
cmake -S portable -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
clang -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -fsyntax-only -In64/tests/stubs -Iportable/include -In64 \
  n64/main.c n64/wireframe.c
```

Phase 8 initial development: five CTest suites passed locally with GCC,
Clang + ASan/UBSan; native N64 execution requires the GitHub MIPS64 cross-build
and a fresh user test on ares. A successful cross-build does *not* demonstrate
successful emulator execution. No gameplay completion percentage can be
inferred from this wireframe prototype.

## Fidelity plan

Next recover source `PlayerCtrl` motion constants and per-state transitions
from IL, then tune navigation data against gameplay evidence. Do not quietly
promote these placeholder physics values to authentic Corn Kidz 64 behavior.

## Phase 9 continuation

Near-plane segment clipping is addressed in `reports/phase9_nearplane_clipping.md`.
This is a renderer maintenance fix, not an original-game fidelity milestone.
