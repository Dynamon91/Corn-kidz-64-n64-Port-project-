# Corn Kidz 64 → Nintendo 64: reverse-engineering workspace

**Status (13 November 2026):** binary analysis and **Phase 4 MusicFade C11 reconstruction**, following Data/save and selected GameCtrl behavior. Source field schema verified; native N64 save hardware and ROM not implemented.

**Important:** no N64 ROM or full playable port exists yet. The available
host tests **do not** establish gameplay equivalence.

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
