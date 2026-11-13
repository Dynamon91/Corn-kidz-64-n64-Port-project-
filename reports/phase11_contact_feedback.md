# Phase 11: Collision feedback and camera-rotated corner test

This is a wireframe diagnostic prototype; it is NOT the original Corn Kidz 64 player or physics.

## Changes
- Existing H count remains cumulative **fixed simulation steps touching** the blue temporary pillar.
- New E count increments only when contact changes from absent to present.
- A 10-step (approximately one-third-second) orange CONTACT! banner marks a new collision. Continuous contact also shows a banner and orange pillar.
- Re-enabling collisions while inside the pillar triggers a 12-step EJECTED banner after the established nearest-face ejection, without falsely incrementing impact count.
- START COLL:OFF clears transient collision feedback; B restores position, enabled collisions, H and E counters, and feedback timers.
- Added a camera-yaw=0.9 radians diagonal corner approach and tests for separate collision events, transient feedback decay and ejection.

## Test on ares
1. Start `roms/ck64_diag_phase11_contact_hud.z64`.
2. Push against the blue pillar with collisions ON. H should keep rising, E should increase once per separate approach; CONTACT! should be visible.
3. Release, move away, then collide again. E should increase to two.
4. Press START for COLL:OFF and go inside. Press START again: EJECTED should appear and player should be displaced to a side.
5. Press B; H and E should reset to zero and COLL:ON should return.
6. Approach the pillar corner while using C-left/right to orbit the camera; check wall sliding and perspective.

## Validation limits
The C11 suite tests contact-onset counting and feedback timers, rotated-camera corner movement, existing 3D clipping and previous GameCtrl modules. GitHub Actions separately builds a native .z64 via libdragon. Host or N64 compiler success does not establish ares execution; send video to validate.
