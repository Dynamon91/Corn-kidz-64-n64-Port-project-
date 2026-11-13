# Phase 14 — Source-grounded PlayerCtrl reconstruction

**Work in progress.** This phase reconstructs the ORIGINAL managed source behavior, not the original gameplay level or models. There is no new playable game ROM.

## Input evidence
- Source: user-owned Corn Kidz 64 v1.029, Unity 2021.3.30f1, Mono.
- Local `Assembly-CSharp.dll`: 317,440 bytes; SHA-256 `2165e2972c5c08ff6839a116dcce324d00f0624a90ffd00a2c19d7b7663e29e9`.
- Original DLL was extracted privately from user's 3-part archive and never committed to the public repo.
- 66 PlayerCtrl methods = 90,370 IL bytes / 26,314 decoded instructions.
- `Run` (0x06000294): 76,019 IL bytes / 22,052 instructions. **NOT yet translated**.
- `reports/playerctrl_method_inventory.csv` indexes names, method tokens, IL lengths and instruction counts. No original IL bodies stored.

## Implemented C11 functions mapped to original method tokens
- `Gravity` (0x060002a1): `yVel -= arg1`; clamp to `-arg2`; if `bOnFloor`, force `yVel=-0.1`.
- `ResetPl` (0x060002c8): ONLY `plState=0`, `bNoCtrl=false`, `camType=1`, `zMov=0`, `plVel=(0,0,0)`. It does **not** reset `yVel` or `bOnFloor`.
- `ZXPos` (0x060002ac): return `(x,0,z)`.
- `LookDisable` (0x060002bc), `LookEnable` (0x060002bd), `AttackDisable` (0x060002be): modify one matching flag.
- `CamShake` (0x060002bb) and `SetFallZone` (0x060002c1): store the corresponding float.
- C port guards against null/nonfinite inputs; those checks are PAL safety additions, not claimed Unity original behavior. `ck64_playercore_init` is a **new** convenience method.

## State-dependent Gravity parameters found inside Run
44 call sites with immediate argument pairs. All use terminal speed `0.6f`. Acceleration decrement varies by state:

| decrement | number of call sites |
| --- | ---: |
| 0.020 | 30 |
| 0.010 | 6 |
| 0.003 | 3 |
| 0.012 | 2 |
| 0.015 | 2 |
| 0.030 | 1 |

These are per-method-call decrements; they should NOT be interpreted as metres/second squared or automatically mapped to 30 Hz.

## Verification
9 separate C11 scenario groups cover gravity, floor override, alternate state constants, selective reset, simple original utility methods, rejection of invalid values. GCC/Clang and sanitizers are tested through CI after publication. `ck64_playercore.c` is compiled into N64 diagnostic source for MIPS64 compatibility, but is **not yet called in the game loop**.

## Next
1. Reconstruct `MovPl` (565 IL bytes) and analyze direct call arguments from `Run`.
2. Decode `plState` transitions, then jump/headbutt/climb/wall movement based on the original methods.
3. Compare the actual gameplay reference https://www.youtube.com/watch?v=nIsAuFddStQ to the recovered state machine; only then replace placeholder `ck64_world3d` physics.
4. Convert real assets privately from user's original copy, never commit original commercial files or extracted video frames.

**Progress convention:** Phase 14/50 = 28% of a provisional roadmap, *not* 28% of gameplay or original engine port completed.
