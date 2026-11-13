# Corn Kidz 64 → Nintendo 64 (N64 Port)

Experimental reverse-engineering and native Nintendo 64 reconstruction project.

**Status — 2026-11-13:** Phase 1 binary analysis complete; Phase 2 GameCtrl analysis and portable flow prototype; Phase 3 C11 save-data reconstruction and host tests.

**No native Nintendo 64 ROM (.z64) has been built or tested. This is not a playable port yet.**

## Verified original game architecture

- Source: Corn Kidz 64 v1.029, Windows x86-64, Unity 2021.3.30f1 / Mono / C#.
- The original Assembly-CSharp.dll exposes 193 managed types and 1,105 methods.
- GameCtrl: 21 methods examined; Data: 34 field names and types identified.
- Target: Nintendo 64, MIPS R4300i, libdragon, native rendering/audio/input.

## Portable prototype

The Phase 3 C11 prototype includes a 1,533-byte versioned big-endian save record with CRC32, a GameCtrl flow subset and tests (18/18 passed on desktop GCC/Clang and sanitizer builds). These are **host-only** test results; N64 flash/EEPROM, graphics, audio, collision, levels and executable ROM are not implemented.

Run host tests when source files are available:

```sh
cmake -S portable -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Archive and project policy

The project is based on reverse engineering of a privately supplied game copy. Only independently authored tools, C sources, tests and analytical reports may be included here. Original Unity executable files, third-party game assets, proprietary audio and models must not be committed.

This repository is a backup of the in-progress analysis, **not** an official game release or a finished port. Preserve original logic as closely as the N64 hardware permits, and clearly label verified and hypothetical behavior.

## Roadmap

1. Native N64 toolchain build via libdragon.
2. Complete platform-independent GameCtrl and Data save layers.
3. PlayerCtrl and camera state-machine analysis/translation.
4. Renderer, audio, level loading and asset adaptation.
5. Emulator and console testing; gameplay-fidelity validation.
