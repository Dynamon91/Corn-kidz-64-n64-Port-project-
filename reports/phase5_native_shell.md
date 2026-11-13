# Phase 5 — 30 Hz game loop + native Nintendo 64 diagnostic frontend

**Date:** 2026-11-13. The code is a *diagnostic shell* for reconstructed GameCtrl and MusicFade state logic, **not a port of Corn Kidz 64 gameplay**.

## Implemented now

- `portable/include/ck64_clock.h`, `src/ck64_clock.c`: allocation-free, integer-only 30 Hz scheduler with explicit 4-update catchup cap, monotonic time guard, and 250ms maximum stall recovery. Works regardless of PAL 50 Hz or NTSC 60 Hz video output if passed real hardware ticks.
- `portable/include/ck64_runtime.h`, `src/ck64_runtime.c`: sequential orchestration of `GameCtrl.Update` counter subset, `GameCtrl.MusicFade` state and `WaitForFade` coroutine step, with error propagation and statistics.
- `portable/tests/test_clock_runtime.c`: host scenarios for NTSC/PAL sampling, backwards timer, long stalls, large clock values, 50-step transitions and audio error boundaries.
- `n64/main.c`: libdragon **diagnostic frontend** demonstrating VI display buffers, controller polling, fixed-step runtime, transition progress, in-RAM save encoding, and visible error status. A/ B/ START are *test controls*, not intended as game controls.
- `n64/Makefile`: N64 cross-build entry point via installed libdragon `n64.mk`.
- `n64/tests/stubs/libdragon.h`: host-only minimal API stub for C type/syntax checking. It cannot validate R4300i linking, RDP or emulator behavior.
- `.github/workflows/n64-diagnostic.yml`: independent MIPS64 cross-build job in the official libdragon container; on success uploads a **diagnostic** `.z64` artifact. No proprietary game data in the artifact.

## Source fidelity / known divergences

- Source `GameCtrl.Awake` targets Unity frameRate 30; driving Update exactly 30 times/s is an *initial* hypothesis for N64 pacing, not identical Unity scheduling.
- Source `WaitForFade` uses 50 update cycles; the existing state machine is reused unchanged. There is no original fade artwork/visual timing beyond state semantics.
- Music model has no original clip data and cannot produce sound; preview UI toggles only the music flag.
- The dummy scene names `diagnostic` / `test_room` are a harness; no original scene loading exists.
- Save records are 1,533-byte RAM scratch snapshots only. EEPROM and Controller Pak I/O are not active, so saves do not persist after reset/power off.
- While original Unity data is required for actual levels/models/animations, proprietary files must stay off the public GitHub repository.

## Host verification commands

```sh
cmake -S portable -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host --parallel 2
ctest --test-dir build-host --output-on-failure
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -fsyntax-only -In64/tests/stubs -Iportable/include n64/main.c
python3 tools/verify_phase4_musicfade.py
```

## N64 native cross build

Requires **installed** libdragon trunk, toolchain and `N64_INST`:

```sh
make -C n64
```

Alternatively, GitHub Actions `N64 diagnostic ROM build` uses `ghcr.io/dragonminded/libdragon:trunk`, installs the SDK into the container, and executes Make. The output is named `n64/ck64_diag.z64`. Only a successful workflow proves a cross-compiled diagnostic ROM exists. A run in Ares/console is still required to validate actual execution. Do not treat the existence of a ROM header as proof that gameplay works.

## Working set next

- Asset format inventory and platform conversions under N64 memory limits.
- Recovery of PlayerCtrl physics/input and camera zone state.
- Actual RDP/RSP 3D rendering and scene graph.
- Real music sample/track streaming and persistent save-media backend.

## Actual GitHub Actions results (13 Nov 2026)

- Compiler package `gcc-toolchain-mips64-amd64.deb` installed successfully in Ubuntu 24.04 runner.
- libdragon trunk compiled and installed; native linker completed; ROM header check passed; ROM uploaded as artifact.
- Build CI: https://github.com/Dynamon91/Corn-kidz-64-n64-Port-project-/actions/runs/43395753964 (success).
- Host CI: https://github.com/Dynamon91/Corn-kidz-64-n64-Port-project-/actions/runs/43395754126 (success).
- Extracted artifact: 147,456-byte `ck64_diag.z64`; SHA-256 `c3f6f340006a4e1945c6ac060027896f14a32c37afc3e7e1cfeda7a575873bdc`; N64 magic `80371240`; internal title `CK64 Port Diagnostic`.
- No emulator/real hardware test has been run. Status is diagnostic ROM *compile verified*, not runtime verified.
