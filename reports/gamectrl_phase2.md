# Corn Kidz 64 → Nintendo 64: GameCtrl Phase 2 checkpoint

Date: 13 November 2026. Scope: **static-only, source-grounded cleanup** of selected `GameCtrl` behaviors from the user's uploaded Windows Unity 2021.3.30f1 build. No Unity binaries were executed. No emulator/N64 build was attempted.

## Recovered evidence

The stdlib-only `tools/inspect_gamectrl_il.py` reads the managed PE CLI metadata, resolves field and MemberRef tokens, decodes IL and validates branch targets. For `GameCtrl`, this produced **770 instructions in 21 methods**, written to `reports/gamectrl_il.json`. For `<WaitForFade>d__44`, the generated `reports/wait_for_fade_il.json` includes the coroutine state machine. The separate evidence checker validates **15 static assertions**. This is an *IL disassembly*, not a high-level or verified 1:1 C# decompilation.

| Source method | Verified IL behavior | Portable work |
|---|---|---|
| `GameCtrl::.ctor`, `Start` | `startHP=5`; `HP=startHP`; `DP=16` | Initialized in `ck64_gameflow_init` |
| `ResetSav` | `currentLvl=1`, `maxHP=8`, arrays `items` and `switches` 1000 booleans, `upgrades` 100 booleans, three int arrays of 100; many settings zero | Only scalar subset represented. Full `Data` not implemented |
| `Awake` | sets targetFrameRate=30; creates 99-element level thresholds; threshold positions 1–7 = `0,70,140,200,300,360,999` | Threshold table recovered; no Unity lifetime management |
| `SetCurrentLvl` | increments current level by **one** iff `itemTotal >= lvl[currentLvl+1]` | Source-derived conditions with out-of-bounds guard |
| `SetVolume` | `data.volume = arg`, listener gain = `float(volume) * 0.01f`; no clamp | Source-derived computation, downstream audio backend pending |
| `LoadScene` | save first; reset fade flag; empty string reloads current scene; on nonempty target, remember current scene if it isn't `gameover`; set `sceneTimer=0` | Callback-driven scene/save operation |
| `SetCheckpointScene` | stores current scene name | Source-derived operation |
| `FadeToScene` | skip when fading, choose checkpoint door for zero-valued door, resolve special-world return, invoke UI `transOut` and start `WaitForFade` | Source-derived guards and transition initiation |
| `Return` | skip when fading; return to lastScene/lastDoor; set nextTrack=1; invoke transition | Source-derived operation |
| `Die` | clamps negative checkpoint door; special-world return; otherwise reset HP, increment deaths, request `gameover` transition | Source-derived operation |
| `<WaitForFade>d__44::MoveNext` | sets `bFading`, yields while counter is below **50**, resets `currentWorld` to zero then calls `LoadScene` | 50-tick transition at host frame-tick granularity |
| `SaveGame` / `LoadGame` | BinaryFormatter serialization to `<persistentDataPath>/save1.cow` | **Not yet implemented**; save/load callback is only an interface |

Additional available call names include `UnityEngine.Input.GetKeyDown`, `Mathf.MoveTowards`, `UnityEngine.AudioSource`, Steamworks and SceneManager. `MusicFade`, `Update` and other significant methods have not been refactored. The disassembler leaves some MethodSpec generic references (`0x2B...`) unresolved; a future step can resolve MethodSpec and full signatures.

## Deliverables and reproducibility

- `portable/include/ck64_gameflow.h`, `portable/src/ck64_gameflow.c`: portable C11 scene/progression state, with no allocation or Unity/libdragon dependency.
- `portable/tests/test_gameflow.c`: nine host test scenarios for original conditions, edge cases and callback failures.
- `portable/CMakeLists.txt`: host-only CMake build for testing the clean core.
- `tools/inspect_gamectrl_il.py`: CLI IL decoder (stdlib only).
- `tools/verify_phase2_evidence.py`: verifies assumptions against IL, not in-game behavior.
- `reports/gamectrl_il.json`, `reports/wait_for_fade_il.json`, `reports/gameflow_evidence_checks.json`: reproducible traces.

Host verification completed:

```sh
cmake -S portable -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
# 1 CTest suite passed; suite executes nine scenario groups.
# Also compiled/run using gcc 14.2 with -fsanitize=undefined -fno-sanitize-recover=all.
```

## Explicit differences from Unity semantics / open risks

1. **No 1:1 behavioral proof yet.** Tests compare intended rules against static IL evidence, not a trusted runtime reference or actual controller interaction.
2. **Save file compatibility.** Original uses BinaryFormatter, which is neither portable nor suitable for an N64 runtime. No `.cow` parser, serializer, actual save device or imported save data exists. The C struct contains **only five** scalar fields; it cannot be used to represent the entire `Data` class.
3. **External failure policy.** Unity's original `LoadScene` does not return status; the portable API aborts on failed save and reports backend failures. This is a deliberate safety improvement, not exact error-flow matching.
4. **Scene-name bound.** Managed strings can be arbitrary length; the portable code uses 128-byte buffers and returns an error on overflow. Future asset indexing will replace scene strings with compact numeric IDs after the scene table is recovered.
5. **Time model.** Coroutine `MoveNext` confirms 50 yields but total milliseconds depend on Unity scheduling and the specific frame rate. `ck64_transition_tick()` models one continuation per frame; a precise run-time trace is still required to confirm transition timing.
6. **Unity object truthiness.** `GameCtrl.Return` and `Die` handle managed string nullability in Unity; portable code treats empty `last_scene` as invalid rather than allowing a bad transition.
7. **Unity side effects missing.** No actual camera follow, 3D render, transitions visuals, music player, material logic, collision or asset-loader exists yet. The `transition_out`, `save_game` and `load_scene` backends are test doubles.
8. **Uninitialized level thresholds.** `Awake` initializes only indices 1–7 of a 99-element table, others default to zero. This behavior is preserved, but out-of-range currentLvl values now return a safe error to avoid undefined behavior.
9. **N64 toolchain unverified.** libdragon cross-compiler, linker, Ares emulator and real console have not been run. PC CMake success is not a playable ROM.
10. **Untrusted source package.** Original archive contained third-party executables/Steam emulation components; no executables or game assets are redistributed with this package.

## Coverage and next milestone

`GameCtrl` and its iterator were inspected fully at the **IL instruction** level, but only around **8 of 21 GameCtrl method behaviors** are represented in the portable scene/progression core. This ratio is only a *method-count indicator*; it is **not** a credible whole-port completion percentage. **Playable N64 ROM progress: 0% verified.**

**Next module:** fully reconstruct `Data` serialized field types and reset defaults, then implement a versioned, explicitly big-endian N64 save format and host cross-endian tests. Continue `Update`/`MusicFade` and scene object mapping before attempting ROM compilation. Save format must be authored from verified field semantics instead of assuming a direct copy of .NET binary blobs.
