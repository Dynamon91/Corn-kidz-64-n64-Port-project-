# Phase 15 — Source-grounded PlayerCtrl::MovPl reconstruction

**Project in development. No playable original Corn Kidz 64 port yet.**

Original user-owned Unity v1.029 assembly: `PlayerCtrl::MovPl` (token `0x060002a0`) contains **565 IL bytes / 170 instructions** and is called **33 times** from `PlayerCtrl::Run`. Flag-pair counts from static proximity analysis: arg5/arg6 false/false: 2; false/true: 19; true/true: 12. One speed-scale argument is computed dynamically from `yVel`.

Reconstructed method operations:
1. Rotate movement input into the floor plane using `Quaternion.FromToRotation(transform.up, floorNorm)`.
2. **Direct mode (arg5 true):** normalize `ctrlDir` above magnitude 0.6; accelerate `zMov` using `Mathf.MoveTowards`, brake with `Mathf.Lerp`; calculate `plVel = adjustedInput * zMov * speedScale`.
3. **Inertia mode (arg5 false):** retain horizontal velocity heading; accelerate if below requested input speed, then apply `Vector3.MoveTowards` friction, `Vector3.Lerp(...,0.1)` overspeed correction and update `zMov`.
4. Rotate toward `ctrlRot` by fixed angular step or fraction of remaining angle (arg6).
5. Restore world-up orientation using `Quaternion.FromToRotation(transform.up, Vector3.up)`.

The C11 version reproduces IL operation ordering, including per-call values rather than invented 30 Hz scaling. Unity exact trig/float semantics have not been demonstrated to match numerically; safe rejection of negative rates, zero speed scale, and NaN is a PAL addition. `CK64PlayerMotion` is isolated from `CK64World3D` and is **not wired into N64 gameplay**. The former diagnostic behavior is preserved.

Nine portable scenario groups: direct acceleration, damping and 0.6 threshold; inertia acceleration, friction and overspeed; fixed/fractional quaternion rotation; sloped floor and upright correction; atomic invalid-value rejection. GCC, Clang and ASan/UBSan tested locally, and CI will verify repository integration.

Next: decode the actual `Run` state machine that supplies `ctrlDir`, `ctrlRot` and motion parameters; compare real PC movement video; only then enable the new method in ares.

**Roadmap:** 15/50 provisional milestones = 30% of planning phases, **not** 30% of game completion. Do not distribute original game assemblies, raw IL or proprietary art.
