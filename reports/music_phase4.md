# Phase 4 — `GameCtrl.MusicFade` IL-to-C reconstruction

**13 November 2026. Scope:** Implement the deterministic two-channel mixing/control logic in a platform-independent C11 module. No proprietary game audio, Unity binary, SDL, libdragon or original assets are distributed here. This is not an N64 audio engine.

## Evidence from original binary

- The original method contains **570 bytes of IL and 196 decoded instructions** (`reports/gamectrl_il.json`, `GameCtrl::MusicFade`).
- The `GameCtrl::Update` method calls it once per Update (`IL offset 74`). The source game sets Unity target frame rate to 30 in `Awake`; actual real-world scheduling can vary.
- Music `[0]` moves its volume toward **0** when `nextTrack != currentTrack` or `subTrack != 0`, otherwise toward **1**. Both directions use **0.04 per call** (`MusicFade` offsets 0–106).
- When `bMusicAutoSwitch` is set and channel 0's volume reaches exactly **0**, the code clears the flag, copies `nextTrack` to `currentTrack`, assigns `tracks[currentTrack]` to the main `AudioSource.clip`, then invokes `Play` (offsets 106–192).
- When `subTrack != 0`, music `[1]` fades toward **1**. Otherwise, if it is currently playing, it fades toward **0** and calls `Stop` upon reaching exactly 0 (offsets 192–355).
- When music `[1]`'s pitch is unequal to `musicSpeed`, **both** channels move their pitches toward that speed by **0.02 per call** (offsets 355–460). This odd asymmetric gate is copied on purpose.
- When `subTrack == 0`, `musicSpeed` is reset to **1** and channel 0 pitch moves toward **1** by 0.02 per call, in addition to the preceding possible pitch movement (offsets 460–520).
- When `Data.bMusic` is false, channel 0 and 1 volumes are forced to 0 **at the very end**. `Play`/`Stop` can already have occurred earlier in the same call (offsets 520–569).
- `tools/verify_phase4_musicfade.py` passes **19 IL structural checks** against the original decoded method, including constant positions, API calls, branch predicates and hard-mute paths. Structural evidence does not prove complete behavioral equivalence.

## Implementation

- `portable/include/ck64_music.h`, `portable/src/ck64_music.c`: source-grounded pure-state C11 model.
- `portable/tests/test_music.c`: 12 distinct host scenario tests, including first 25 fades, edge conditions, `Play` and `Stop` events, music disabled, pitch gate asymmetry, malformed clips, invalid floats and null pointers.
- A call to `ck64_music_step` corresponds to *one GameCtrl MusicFade invocation*, not a wall-clock second. The N64 frontend must drive calls at the appropriate game-frame frequency and send generated `main_play` / `sub_stop` events to an audio driver.
- `track_count` is provided by the scene/music frontend; the source Unity clip array length and asset mapping are not yet reconstructed. Invalid index returns an error with no state modification.
- State initialization is **provisional** (pitch = 1, volume = 0, music enabled). Unity scene-serialized default values must be populated from actual scene data for parity.
- C state-machine holds only clip indices; audio decoding, sample buffering, music synchronization, format conversions, resource lifetime and N64 RAM budgets remain future tasks.

## Host validation (no N64 ROM)

Validated with GCC 14.2, Clang 17, plus Clang AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
cmake -S portable -B build-phase4 -DCMAKE_BUILD_TYPE=Debug
cmake --build build-phase4
ctest --test-dir build-phase4 --output-on-failure
python3 tools/verify_phase4_musicfade.py
```

Host CTest suites: 3/3 (10 gameflow + 8 save + 12 music scenario groups = **30 passed**).

**Target status:** No Nintendo 64 ROM has been built, no RSP/RDP graphics code has been integrated and no audio driver has been tested. libdragon/mips64 cross-compiler are not available in this environment.

## Next tasks

1. Recover the game track table, expected clip format and `MusicZone` logic from metadata/scene serialization.
2. Separate C state update from a libdragon-compatible audio mixer, with explicitly bounded RAM and independent clip identity.
3. Reconcile the timing model with Unity frames and implement fixed-step scheduling in the N64 loop.
4. Continue `PlayerCtrl` and camera behavior recovery.
