# Corn Kidz 64 — 110% walkthrough reference (original PC game)

**Status:** reference indexed for future source-to-gameplay comparison; **not** yet reviewed frame-by-frame. This is a reverse-engineering reference, not evidence that the original levels or movement have already been ported.

- Original public video: https://www.youtube.com/watch?v=nIsAuFddStQ
- Title: **Corn Kidz 64 - 110% WALKTHROUGH - NO COMMENTARY - [PC HD]**
- Creator: SUBWAY CIRCUIT
- Published: 2024-01-01
- The author describes a full 110% walkthrough including collectibles and secrets.
- Source metadata: YouTube's publicly indexed description and chapter list, retrieved 2026-11-13.
- Copyright notice: link to video only; **do not upload, rehost, or commit the creator's full recording or extracted game assets** without appropriate permission.

## Original video chapter/navigation index

| Video time | Chapter listed by creator |
| --- | --- |
| 00:00 | Main Game |
| 11:14 | Trash 1 |
| 23:30 | Trash 2 |
| 25:45 | Trash 3 |
| 35:48 | Trash 4 |
| 01:48:39 | Wolloh's Hollow — Void Screw #2 |
| 03:45:10 | Trash 5 |
| 03:47:51 | Trash 6 |
| 03:53:52 | Trash 7 |
| 03:58:59 | Trash 8 |
| 04:49:07 | Collecting the rest of EXP |
| 05:27:54 | The Park — Void Screw #3 |
| 05:29:22 | The Park — Void Screw #1 |
| 05:29:45 | The Park — Void Screw #2 |
| 05:31:50 | Anxiety Tower — first run |
| 05:40:21 | Anxiety Tower — Void Screw #1 |
| 05:54:11 | Anxiety Tower — successful complete run |
| 06:09:26 | Anxiety Tower — Void Screw #2 |
| 06:13:12 | Wolloh's Hollow — Void Screw #1 |
| 06:15:55 | Wolloh's Hollow — Void Screw #3 |
| 06:19:29 | Wolloh's Hollow — Void Screw #4, part 1 |
| 06:21:53 | Wolloh's Hollow — Void Screw #5 |
| 06:23:58 | Wolloh's Hollow — Void Screw #4, part 2 |
| 06:51:27 | Secret Area |
| 06:51:58 | Secret Area — Void Screw #1 |

The video's creator has additional 'Main Game' and 'Collecting the rest of EXP' subchapters between these markers. Timestamps are navigation aids only; do not treat them as complete scene boundaries.

## Proposed analysis workflow

1. Reconstruct the movement state machine from the owned Unity assembly's `PlayerCtrl` methods, then compare sampled video moments for idle/walk/run/jump, headbutt, climbing, wall contact, fall and landing.
2. Establish representative source-game camera paths and scene transitions; preserve behavior from original code and verify using footage.
3. Inventory observable gameplay zones and their entry/exit behavior, hazards, collectibles, enemy interactions, HUD/counters and checkpoint semantics.
4. Extract asset data **from the user's supplied original game files** where allowed, not from compressed video frames. Gameplay frames are reference images, not usable 3D models.
5. Create versioned acceptance clips / test cases using new N64 builds in ares. Label **verified, inferred, prototype** clearly.

## Current development reality

Latest validated work in conversation: Phase 13 N64 diagnostic scene with a native libdragon build, tested in ares. That prototype has a wireframe test room and artificial controls/collision; **no authentic Corn Kidz 64 level/model, full `PlayerCtrl` translation or playable port exists yet.**

The previously shown “26%” bar represented phase 13 of a *provisional 50-phase plan* (13/50), **not actual gameplay completeness**. Do not use it as an engineering completion claim.

## First comparisons to record

- Video start (00:00 onwards): title-to-game flow, starting movement and HUD.
- 11:14, 23:30, 25:45, 35:48: transitions into early Trash activities.
- 01:48:39: Wolloh's Hollow secret collection.
- 05:31:50–06:13:12: Anxiety Tower first/successful runs, camera behavior and obstacle tuning.
- 06:51:27: final secret area.

**No frame-accurate observations made yet.** Document movement timing and scene geometry only after examining the relevant footage or original asset data.
