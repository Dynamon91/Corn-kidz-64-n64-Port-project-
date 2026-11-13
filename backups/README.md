# Phase 3 complete backup

This contains all 31 files from `CornKidz64_N64_Phase3_SaveSystem_2026-11-13.zip`, re-packed as `.tar.xz` to preserve the 2026-11-13 host-only C11 source, scripts, reports and evidence JSON.

Extract:

```sh
mkdir -p cornkidz_n64_workspace
tar -xJf backups/CornKidz64_N64_Phase3_full.tar.xz -C cornkidz_n64_workspace
cmake -S cornkidz_n64_workspace/portable -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The original game binaries/assets are not included. The C11 prototype has not been compiled for Nintendo 64.

SHA-256 of tar.xz: `a6465249df7e24411f66a7832bd8b1d7da0de0664d375828ccd7ee9c77a158a2`.
