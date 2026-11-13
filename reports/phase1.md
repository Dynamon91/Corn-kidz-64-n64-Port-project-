# Corn Kidz 64 (Windows Unity) → Nintendo 64: Phase 1 technical report

Date: **2026-11-13**. This report reflects static analysis of the three
user-provided split ZIP parts. No executable was launched.

## 1. Archive integrity and provenance

The files `.001`, `.002`, and `.003` form a sequentially split **ordinary ZIP**
file, not three independent archives. After byte-for-byte joining:

| Metric | Observed |
|---|---:|
| Combined ZIP bytes | 133,722,631 |
| Archive entries | 232 |
| Regular files | 213 |
| Total uncompressed entry bytes | 244,794,370 |
| ZIP CRC validation | All entries pass (`testzip() == None`) |
| SHA-256 | `2d1f4f33ff4cfc86d77a71649c5f89d7191d93c2fa6ad79096676e3aec3c459f` |

The archive includes `steam_emu.ini` and a SteamUnlocked URL shortcut. This
is not a clean provenance signal; the package contains third-party components.
The original executable has **not** been run or trusted. Static examination
should focus on the game's managed code and Unity content. Do not bundle,
redistribute, or depend on third-party emulation/DRM files for a native port.

## 2. Actual source architecture

| Item | Evidence and implication |
|---|---|
| Host platform | Windows x64 (`Corn Kidz 64.exe` is PE32+ AMD64) |
| Managed game code | `Assembly-CSharp.dll`: CLI PE32 metadata, not 32-bit native gameplay code |
| Engine | Unity **2021.3.30f1** identified inside `globalgamemanagers` |
| Runtime | Mono (archive contains `MonoBleedingEdge/EmbedRuntime`) |
| Custom code inventory | **193** types, **1,105** methods, **230,124 bytes of IL** |
| Native entrypoint dependencies | `UnityPlayer.dll`, `KERNEL32.dll` from PE import table |
| Other plugin dependencies present | `steam_api64.dll`, `XInputInterface.dll`, Facepunch.Steamworks, XInputDotNetPure |
| Unity content | `level0`–`level17`, `sharedassets*.assets`, `.resS`, `.resource` |
| Unity serialized-file version | **22** in sampled serialized file headers |

`Assembly-CSharp.dll` uses .NET metadata version `v4.0.30319`. This is a CLR
metadata version marker, **not** the Unity editor version. Likewise, the
`PE32 / i386` container reported by `file` for the managed DLL does **not**
mean that the game's logic consists of x86 instructions: the methods contain
Common Intermediate Language (IL).

Unity scenes `level0`–`level17` are **serialized scene records**. Their count
is not a confirmed count of independent playable levels. Resolving human
scene names and map ownership requires reading serialized scene objects.

The source runtime is little-endian on AMD64. N64's R4300i normally uses
big-endian data conventions; all future binary readers and asset encoders
must explicitly handle byte order. The libdragon toolchain supports the
R4300i's 64-bit facilities; pointer layout must be checked against the
selected cross-compilation ABI, not simply assumed to be 32-bit.

## 3. First subsystem inventory

| Type | Method count | Significant methods from CLI metadata |
|---|---:|---|
| `PlayerCtrl` | 66 | `Run`, `MovPl`, `Gravity`, `LedgeGrab`, `GrabPole`, `DrillCharge`, `Bounce`, `Hurt`, `ChangeScene` |
| `GameCtrl` | 21 | `SetCurrentLvl`, `LoadScene`, `SaveGame`, `LoadGame`, `MusicFade` |
| `Bad` | 41 | `Collisions`, `StateLoop`, `StateAttack`, `StateHurt`, `StateDie` |
| `FixCamZone` | 9 | `OnTriggerEnter`, `OnTriggerStay`, `OnTriggerExit` |
| `MusicZone` | 9 | `SetMusic`, `PlayMusic`, trigger enter/stay/exit |
| `UI` | 43 | `RunMenu`, `LoadHUD`, `Ctrls`, `Pause`, `RunText` |
| `World` | 5 | `SetCheckpoint`, `Awake`, `Start` |
| `SaveTrigger` | 7 | `Load`, `SetData`, lifecycle methods |

The `PlayerCtrl.Run` IL body is **76,019 bytes**, so this is a major
reconstruction bottleneck. Replacing it with a guessed movement controller
would not produce an accurate port.

Important distinction: type/method metadata identifies *where to work*;
control flow, constants, collision semantics and data layouts have **not yet
been reverse-engineered**. No C function is asserted to be equivalent.

## 4. Size/memory observations

The source archive contains approximately **100,009,258 bytes** of Unity
serialized content based on file-name classification (including streamed
resources). This is not equivalent to texture or geometry size after decode.
A single stream `sharedassets0.resource` is 30,889,714 bytes. The full
Windows/Unity runtime, compiler DLLs and bundled redistributables are **not**
N64 runtime dependencies and must not be transplanted.

A standard N64 provides 4 MiB of RDRAM; with Expansion Pak, 8 MiB. The
hardware RDP has very limited on-chip texture memory. Because Unity scene
and asset storage is far larger than N64 working memory, the port will need
quantized mesh/texture data, streamed scene chunks, bounded active-object
sets, and a deliberately budgeted audio pipeline. The exact target ROM size
and fidelity trade-offs remain unmeasured until asset/object extraction.

## 5. N64 target architecture (proposed, not yet built)

- **Native runtime**: C11 + libdragon; test with a hardware-accurate emulator
  such as Ares. Use a pinned SDK revision after build tooling is available.
- **Game-specific portable core**: deterministic fixed-step state updates,
  entity states, collisions, checkpoint and save state, camera modes.
- **PAL backends**: N64 input/controller, RDP/RSP graphics, cart ROM data
  access, audio mixer, save devices, timer, profiling and diagnostics.
- **Rendering**: replace Unity shaders, lights and material graph with N64
  RDP draw state and reduced/material-baked equivalents.
- **Scene loading**: translate serialized Unity object/transform references
  to an N64-oriented scene data format, loaded in bounded segments.
- **Controller**: map Unity/XInput input semantics to N64 analog stick,
  A/B/Z/R/L/C-buttons. Preserve original interaction semantics and
  camera behavior where possible; mapping is not yet validated.
- **Audio**: identify actual source clip payloads and loop/cue metadata,
  then determine suitable resampling and mixing formats under N64 budgets.

### API replacement inventory

| Source | Destination concept | Notes |
|---|---|---|
| `UnityEngine.Input` / XInput | libdragon N64 controller polling | Dead zones/press semantics to recover |
| Unity `Transform` | Portable transform graph | World/local hierarchy and update order matter |
| Unity Physics / `CharacterController` or raycasts | N64 collision and movement subsystem | Must reconstruct exact state transitions |
| Unity `Camera` / timeline controls | Native camera state machine | `FixCamZone` and scripted camera controls |
| Unity `SceneManager` | ROM scene loader | No embedded CLR/Unity runtime on N64 |
| Unity AudioSource / AudioListener | Native audio driver/mixer | Preserve cue timing and music zones |
| Unity save preferences | Save-device abstraction | Save Pak/SRAM/FlashRam design to follow |
| Steam achievements / Steamworks | Optional no-op/interface | No Steam API on standalone N64 |

## 6. Risk register

1. **Logic fidelity:** `PlayerCtrl.Run` is enormous; decompile to a control-flow
   graph, state transitions, field names and constants before a native rewrite.
2. **Unity references:** MonoBehaviour methods depend heavily on serializable
   fields and scene prefab links. Pure IL without scene objects is insufficient.
3. **Endianness/ABI:** never cast binary file bytes to native structs without
   explicit endian conversion; use fixed-width integer types and checked offsets.
4. **Real-time budget:** N64 CPU, RDP texture memory and RDRAM are drastically
   below PC Unity budgets. Quantify per-scene peaks before choosing textures,
   shading and visibility budgets.
5. **Game archive provenance:** avoid running repack executables and avoid
   shipping proprietary game resources or DRM-related files.
6. **Toolchain not validated:** no libdragon build, ROM generation, emulator
   execution or real-hardware test has yet been performed.

## 7. Next work items, respecting the five-phase workflow

**Continue Phase 1:** parse field signatures and type relationships, enumerate
actual external method calls, recover scene names and serialized references,
identify original input and timing constants, and verify a clean game build if
available. A secure static-only workflow is preferred for this archive.

**Phase 2, first small module:** recover complete `GameCtrl` IL bodies,
reconstruct `LoadScene`, `SetCurrentLvl`, `SaveGame` and `LoadGame` into
annotated, idiomatic portable C (or a C# intermediate model for validation),
with before/after tests against a controlled reference. Do **not** claim
functional equivalence based on method names alone.

**Phase 2, movement:** split `PlayerCtrl.Run` into a source-grounded state
machine with named states, transition guards and physics helpers. Integrate
`FixCamZone`, `Bad`, and `World` only after dependencies are mapped.

**Phase 3:** PAL interfaces and memory budgets once core state semantics are
verified. **Phase 4:** asset format decoding and N64 conversion. **Phase 5:**
libdragon build, Ares validation, then hardware regression tests.

## 8. References for target platform

- libdragon documentation: https://libdragon.dev/
- RDPQ rendering documentation: https://libdragon.dev/ref/group__rdpq.html
- Corn Kidz 64 official Steam listing: https://store.steampowered.com/app/2575900/Corn_Kidz_64/

The original game's official listing states a 30 FPS default and a 60 FPS
beta option. Until measured from this specific build, **30 FPS is a design
reference, not a validated replay cadence** for the port.
