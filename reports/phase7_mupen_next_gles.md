# Phase 7: Mupen64Plus-Next GLES immediate crash triage

**Target:** RetroArch using Mupen64Plus-Next GLES (exact GLES2/GLES3 and device version not yet established).

**Observed by tester:** Phase 5/6 homebrew diagnostics attempt to launch then crash before a usable diagnostic screen.

## Evidence and possible mechanisms

- The known Mupen64Plus issue [#1185](https://github.com/mupen64plus/mupen64plus-core/issues/1185) documents failure before application code in libdragon IPL3 r5–r10, with the r4 boot-loader region reportedly working when substituted into a test ROM. The issue was open on 2026-11-13.
- Separate Android Mupen64Plus-Next GLES3 startup regressions were discussed in [RetroArch issue #19599](https://github.com/libretro/RetroArch/issues/19599); the reporter says the fix reached a subsequent core update, so updating RetroArch/core and clearing damaged automatic savestates is reasonable before drawing conclusions.
- `GLES` refers to the graphics API, not the CPU core. Changing GLES2/GLES3 alone cannot prove or fix a pre-user-code IPL3 failure.
- The Phase 6 ROM is 147,456 bytes with big-endian `80 37 12 40` header. libdragon IPL3 permits small ROMs with zero header checksums; padding to 1 MiB is not a bootloader fix.

## Experimental boot replacement

The script `tools/splice_ipl3_r4.py` takes the unmodified Phase 6 diagnostic and historically pinned libdragon IPL3 r4 (commit `066e5fc39e`) and swaps **only bytes 0x40–0xFFF**. It checks the old IPL3 MD5 documented in libdragon's historical boot README and the original ROM SHA-256. The modified ROM remains marked *experimental*. There is no guarantee it boots with a particular RetroArch core, device, or Android release; a successful GitHub Actions job only verifies byte-level patching and artifact creation.

The historical IPL3 binary is an upstream libdragon project artifact, not a Nintendo ROM dump or game asset.

## User test protocol

1. First make sure a known-good commercial N64 ROM launches using the same exact RetroArch core; if not, update the core and confirm it works before debugging the new test ROM.
2. In RetroArch, select `Mupen64Plus-Next GLES`. Open **Quick Menu → Core Options → CPU Core**. Try `cached_interpreter` first, then `pure_interpreter` if present, with a full core restart (these modes may still fail and are slower). Restore `dynamic_recompiler` for regular use.
3. Temporarily disable automatic savestate loading, and re-test cleanly, if affected.
4. Compare original `ck64_diag_phase6.z64` versus experimental `ck64_diag_phase6_ipl3_r4_mupen_test.z64` from GitHub.
5. Report exact core name/version, GLES2 or GLES3, RetroArch version, device/OS, whether any `BOOT 1/2/3` message appeared, and if possible the RetroArch verbose log.
6. If possible, repeat the diagnostic ROM in ares or gopher64 for better homebrew timing and IPL3 support.

**Status:** native ROM builds exist, but execution on emulator or real N64 is *not verified*. No player movement, levels, or playable game in this ROM.
