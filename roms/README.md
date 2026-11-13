# Nintendo 64 diagnostic ROMs (not the full game)

These are experimental boot / controller / runtime diagnostics for a native Nintendo 64 port of Corn Kidz 64. No original level, playable character, music or copyrighted asset is included.

| ROM | Purpose | Size |
|---|---|---|
| `ck64_diag_phase5.z64` | First native N64 diagnostic | 144 KiB |
| `ck64_diag_phase6.z64` | Adds BOOT 1 / BOOT 2 / BOOT 3 on-screen checkpoints | 144 KiB |
| `ck64_diag_phase6_ipl3_r4_mupen_test.z64` | Experimental replacement of newer IPL3 boot code with historical libdragon IPL3 r4; created for RetroArch/Mupen64Plus-Next GLES launch-crash diagnosis | 144 KiB |

**Historical libdragon IPL3 r4** is pinned to Git commit `066e5fc39e`, official boot-file MD5 `d78ddd06d2303d3f76d11de6b6326d69`. The source diagnostic ROM SHA-256 is `f4448625206ca37698681b4949cd538dcf5628737d8f14e5f3c12c83e0d185de`. The patched ROM SHA-256 is `97b40c19bf71b396029e03f7f2d947beb9465e5a91ee919da850c09584684d14`.

Patch source: `tools/splice_ipl3_r4.py`; documented triage: `reports/phase7_mupen_next_gles.md`; automated GitHub build `.github/workflows/mupen-ipl3-r4-test.yml`.

The original ROM file was preserved byte-for-byte; only bytes `0x40..0xFFF` are different in the experimental ROM. The header magic is big-endian `80 37 12 40` in both. Header checksums are zero by libdragon design.

**Important:** Successful GitHub Actions tests and valid ROM headers do NOT prove successful execution in RetroArch, ares, or original N64 hardware. Test the experimental ROM against the original using the exact same core and emulator version. If both fail, check whether commercial N64 ROMs launch normally, and update the core.
