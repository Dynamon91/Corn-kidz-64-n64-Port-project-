# Corn Kidz 64 → Nintendo 64: reverse-engineering workspace

**Status (13 November 2026): Phase 5 N64 diagnostic ROM cross-compiled** using the official MIPS64 compiler and libdragon in GitHub Actions; portable clock/runtime and selected GameCtrl/MusicFade state logic are tested. Persistent N64 saves, 3D gameplay and original levels are not implemented.

**Important:** a diagnostic-only native `.z64` ROM now exists. **No playable Corn Kidz 64 port exists yet.** The ROM has not been exercised on Ares/console; host tests do not establish gameplay equivalence.

## Verified source facts

- Uploaded ZIP parts: `Corn.Kidz.64.v1.029 (1).zip.001`, `.002`, `.003`.
- Combined ZIP SHA-256 `2d1f4f33ff4cfc86d77a71649c5f89d7191d93c2fa6ad79096676e3aec3c459f`
- 232 archive entries, all ZIP CRC checks passed.
- Unity 2021.3.30f1 Windows AMD64 game, C#/Mono logic.
- `Assembly-CSharp.dll`: 193 managed types, 1105 methods and 230,124 bytes IL.
- `GameCtrl`: 21 methods / 770 decoded instructions.
- `Data`: 34 field names/types verified from CLI signatures; 20 assigned by ResetSav.
- `<WaitForFade>d__44`: coroutine delaying scene load across 50 frame yields.

## Read the reports first

- `reports/music_phase4.md`: 196-instruction MusicFade method, precise two-channel fades/pitch rules, limitations, 30 tested host behaviors.
- `tools/verify_phase4_musicfade.py`: 19 evidence checks against the original `MusicFade` IL.

- `reports/phase1.md`: original executable, binary/architecture analysis, risks.
- `reports/gamectrl_phase2.md`: Phase 2 behavior scope and limitations.
- `reports/save_phase3.md`: complete save record, reset behavior, source-grounding and test results.
- `reports/data_fields.json`: all original Data field definitions and types.
- `reports/save_phase3_evidence.json`: 13 passing metadata/IL checks.
- `reports/gamectrl_il.json`: decoded method instruction streams and resolved tokens.
- `reports/wait_for_fade_il.json`: coroutine instruction stream.
- `reports/gameflow_evidence_checks.json`: 15 passing static checks.
- `reports/assembly_inventory.json`: whole managed assembly method catalog.

## Host-only portable logic and tests

```sh
cmake -S portable -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

This builds the C11 `ck64_gameflow` static library on a desktop compiler, not
an N64 ROM. It now includes the 34-field `CK64SaveData` snapshot, a 1,533-byte
versioned big-endian CRC32 record, a six-field GameFlow synchronization bridge,
and a source-faithful selective `ResetSav`. GameCtrl also covers scene changes,
progression, death, checkpoints, volume and Update's counter/HP subset. N64 IO
and graphics/audio backends are deliberately not implemented yet.

`portable/include/ck64_music.h` and `portable/src/ck64_music.c` reconstruct
`GameCtrl.MusicFade` as a portable, two-channel state machine from source IL.
Actual game music samples and N64 playback are **not** included.

Host test counts: 10 gameflow + 8 save + 12 MusicFade scenario groups = **30 passing**. The music state machine emits Play/Stop and clip-switch events, not audible sound.
AddressSanitizer and UndefinedBehaviorSanitizer host builds also pass.

## Repeat IL inspection with a personal copy of the game

```sh
python3 tools/inspect_gamectrl_il.py '/path/to/Assembly-CSharp.dll' \
  --type GameCtrl --output reports/gamectrl_il.json
python3 tools/inspect_gamectrl_il.py '/path/to/Assembly-CSharp.dll' \
  --type '<WaitForFade>d__44' --output reports/wait_for_fade_il.json
python3 tools/verify_phase2_evidence.py reports/gamectrl_il.json \
  reports/wait_for_fade_il.json
```

`tools/dotnet_metadata_inventory.py` and `tools/scan_split_zip.py` reproduce
the earlier structural checks. Inspect the full save Data field schema via:

```sh
python3 tools/inspect_data_fields.py '/path/to/Assembly-CSharp.dll' \
  --output reports/data_fields.json
python3 tools/verify_phase3_evidence.py .
```

All inspection scripts are pure Python 3.9+ stdlib and **never execute the game's binaries**.

## Distribution policy

This download includes **only independently authored C code, Python tools,
IL data extracts, and reports**. It does **not** ship Unity binaries,
proprietary models, audio, textures, original game resources or Steam emulation
components. The game must be obtained separately by its lawful owner.

## Phase 5 — Native N64 diagnostic shell (not gameplay)

A **30 Hz fixed-step** portable clock/runtime (`portable/include/ck64_clock.h`, `ck64_runtime.h`) now calls the selected `GameCtrl` counter/transition methods and the `MusicFade` state-machine once per update. Unit tests cover 50/60 Hz render cadence and delayed scene transitions.

`n64/main.c` is a **libdragon diagnostic screen**: it displays counters/transition state and reads Controller 1. Its scene names are placeholders and its save record is **RAM only**; music is **silent**. It does not render, load or run original levels.

Host: `cmake -S portable -B build && cmake --build build && ctest --test-dir build --output-on-failure`.
Native (after installing libdragon): `make -C n64`. The GitHub Action `N64 diagnostic ROM build` has **successfully cross-built** `ck64_diag.z64` and published it as a downloadable workflow artifact (14-day retention). Even a successful ROM build **does not prove execution on an emulator or actual gameplay**. See `reports/phase5_native_shell.md`.


## Verified Phase 5 native build (GitHub Actions)

- Build commit: `a3218960fd7c455ca7193c19e0df16094771407f`
- N64 build run: https://github.com/Dynamon91/Corn-kidz-64-n64-Port-project-/actions/runs/43395753964 — **success** (MIPS64 compiler, libdragon build, linker, ROM magic, upload).
- Host C11 run: https://github.com/Dynamon91/Corn-kidz-64-n64-Port-project-/actions/runs/43395754126 — **success** (GCC, Clang, sanitizers and frontend syntax).
- ROM filename: `ck64_diag.z64`; length **147,456 bytes**; begins with N64 big-endian marker `80 37 12 40`.
- ROM SHA-256: `c3f6f340006a4e1945c6ac060027896f14a32c37afc3e7e1cfeda7a575873bdc`.
- Artifact: `ck64-diag-n64-rom`, downloadable from the N64 workflow run while retained.

This establishes that the prototype is a well-formed cross-built ROM *image*, **not** that it boots/runs correctly in an emulator or real hardware. No original game assets are shipped.

## Phase 6 — RetroArch crash triage

A user reported that the Phase 5 ROM crashes on RetroArch during startup. A
known Mupen64Plus incompatibility with libdragon IPL3 r5+ may explain the
behavior (upstream issue #1185); the exact core is still unknown. This does
**not** prove our program is fault-free. The phase6 diagnostic prints boot
stages and the N64 CI now produces a 1 MiB **experimental** padded ROM alongside
the default ROM. **Padding does not fix broken IPL3 emulation.** See
`reports/phase6_retroarch_crash.md` for the evidence and test matrix.

For static ROM audit:

```sh
python3 tools/n64_rom_compat.py roms/ck64_diag_phase5.z64 \
  --pad-output /tmp/ck64_diag_phase5_1mib_test.z64
python3 -m unittest discover -s tools/tests -v
```

Do not report the boot crash as fixed until the user confirms an on-screen boot
marker on a real emulator, and/or a true headless emulation test has passed.
