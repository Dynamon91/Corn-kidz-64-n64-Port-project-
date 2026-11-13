# Phase 6 — RetroArch crash triage

**User report:** Phase 5 diagnostic .z64 starts loading under RetroArch, then the emulator crashes immediately. RetroArch core/version, platform, stack trace and video log are not yet provided; root cause cannot be proven.

## Reproduced static findings

- Phase 5 diagnostic ROM was cross-compiled in GitHub Actions; previously no emulator execution was verified.
- Phase 5 SHA-256: `c3f6f340006a4e1945c6ac060027896f14a32c37afc3e7e1cfeda7a575873bdc`.
- Its first bytes are `80 37 12 40` (big-endian .z64), length 147,456 bytes (144 KiB).
- CRC32 for `[0x40, 0x1000)` is `0x345e1229`, the **IPL3 r8** prefix reported in reverse-engineering reference material.
- Upstream mupen64plus-core issue #1185, filed 2026-08-12, reports that libdragon IPL3 r5 and newer can fail *before `main`* on Mupen64Plus. This matches the symptom **if** the user's RetroArch core uses affected Mupen64Plus code.
- ROM size below 1 MiB can cause additional compatibility problems with some emulator/flashcart configurations, but padding does **not** fix an IPL3 CPU/emulation bug.

## Code changes

- `n64/main.c` now draws boot stages after initializing the display, then the controller, then the runtime; if the first text never appears, the failure is earlier than game logic (or the frontend cannot display video).
- `tools/n64_rom_compat.py` audits N64 magic, SHA-256, IPL3 prefix CRC, `TOC0` and size, optionally creates a byte-preserving **1 MiB padded test** variant.
- Added tests and a GitHub Actions step to generate, audit and publish both standard and padded diagnostic artifacts.

## Testing priorities

1. Use Ares with Homebrew mode on a capable desktop system; alternatively gopher64 if available for the user's operating system. Upstream libdragon says Ares is the reliable emulator for homebrew.
2. If the user only has RetroArch, record core name/version (e.g. Mupen64Plus-Next or ParaLLEl N64) and whether the whole app crashes or returns to the menu. Try the padded variant only as a control.
3. If the screen shows `BOOT 1/2/3`, inspect stage to isolate a library/frontend issue; if no BOOT banner appears and only Mupen fails, investigate IPL3 issue #1185.
4. Do NOT claim either variant has been tested on a console or emulator without such evidence.

## External sources

- https://github.com/mupen64plus/mupen64plus-core/issues/1185
- https://libdragon.dev/ ("Using emulators")
- https://github-wiki-see.page/m/DragonMinded/libdragon/wiki/Upgrade-troubleshooting (ROM size)

**ROM gameplay completion:** Not estimable; no original gameplay integrated. **Emulator boot verified:** no.
