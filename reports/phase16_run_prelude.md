# Phase 16 — Original PlayerCtrl.Run prelude (source-grounded)

**Project in development. No playable Corn Kidz 64 N64 port yet.**

## Scope and provenance

The original user-owned Corn Kidz 64 v1.029 Unity/Mono Assembly-CSharp.dll was inspected privately. No proprietary DLL, Unity resources, video frames, or raw 22,052-instruction method body are redistributed. The public audit script accepts private IL JSON, checks instruction positions and operations, and emits the non-sensitive structural summary in reports/player_run_prelude_evidence.json.

PlayerCtrl::Run token 0x06000294: 76,019 bytes IL / 22,052 instructions. Existing Phase 14 and 15 work reconstructed selected Gravity and MovPl functions; their 44 and 33 call sites occur inside Run, respectively. **The complete gameplay state machine remains unported.**

## Newly reconstructed, independent C11 module

portable/include/ck64_player_run.h, portable/src/ck64_player_run.c, portable/tests/test_player_run.c:

1. Detect actual plState != changeState entry transitions. On entry, selectively reset plTimer, plTimer2, plTimer3, plCounter, plFloat, bNoHurt, bNoCol, bNoLook, and wallObj; update lastState (old changeState) and changeState (new plState). **Do not reset these on ordinary frames**. Emit flags for visual/physics side effects requiring a future N64 frontend, including stopping sound 53 when leaving state 46 or sound 55 when leaving state 45.
2. When plTimer == 1, clear plSwitch. Calling this per-frame routine **does not advance plTimer**; original state handlers own the timer.
3. Respect rumble countdown by 1 per call, then fade both motors with Unity-style Mathf.MoveTowards until zero. Motor I/O itself is not implemented.
4. While bNoCtrl is true, zero exactly 13 original control fields: Adown, Aup, Ahold, Xdown, Xhold, Bdown, Ydown, Yhold, RTTimer, LTTimer, ctrlX, ctrlY, Atimer. Notably Bhold, RBhold, LBhold, LBdown remain unchanged. Otherwise update Atimer from an already-acquired Ahold flag; the original Unity / XInput input acquisition is **not** replicated in this module.
5. Compute early camType selection: underwater normal camera -> 2; player state 18 -> 3 unless the preceding underwater branch matched; fixed camera 4 and disabled camera 0 have original exceptions; otherwise default -> 1. Actual view transform and interpolation not implemented.

The C11 portability layer adds nonfinite/null/error checks and atomic output on validation failure, which are **not** claimed to be Unity-identical. Rumble values are not N64 controller rumble commands. This module is deliberately **not called by** n64/main.c or ck64_world3d while Unity state dispatch remains incomplete. Compiling it into the diagnostic ROM checks MIPS64 ABI compatibility only.

## Verification

- 10 standalone native C11 scenario groups: initialization, transitions, prior-state sound events, plSwitch timer semantics, selective control lock, A hold timer, underwater camera priority, camera exceptions, rumble countdown/fade, atomic invalid-value rejection.
- GCC -Wall -Wextra -Wpedantic -Werror, Clang + AddressSanitizer/UndefinedBehaviorSanitizer tested locally.
- IL structural audit runs on private original source and produced a public metadata-only summary.
- CI expands existing portable CMake host tests and N64 cross-build when committed.
- **No ares test of newly active original movement is claimed.** Phase 13 user's ares test of earlier wireframe is last on-screen validation.

## Next: Phase 17

1. Partition Run by actual plState comparison/branch edges and group MovPl/Gravity calls into source-grounded movement modes.
2. Reconstruct initial grounded/airborne transitions and button requirements, comparing source and walkthrough footage.
3. Add a visibly labeled **experimental** toggle to N64 only after motion state-transition parity has enough evidence; keep original diagnostic mode as fallback.

**Progress:** 16/50 = 32% of an *illustrative milestone plan*, **not** gameplay/asset completion.
