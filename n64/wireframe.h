#ifndef CK64_WIREFRAME_H
#define CK64_WIREFRAME_H
/* Phase 12 N64 CPU wireframe diagnostics. No original game graphics.
 * Libdragon legacy line primitives are not a hardware 3D backend.
 */
#include <libdragon.h>
#include "ck64_world3d.h"
#include "ck64_perf.h"

/* Order matches the controller R cycle: FULL -> SPARSE -> OFF -> FULL. */
enum { CK64_GRID_OFF = 0, CK64_GRID_SPARSE = 1, CK64_GRID_FULL = 2 };

typedef struct CK64WireframeStats {
    unsigned lines_attempted; /* World-space lines submitted for projection. */
    unsigned lines_drawn;     /* 2D lines surviving projection and clipping. */
} CK64WireframeStats;

/* HUD displays the previous fully presented frame's G/D/S/total call
 * durations to avoid conflating unfinished current-frame work with complete
 * measurements. Samples are not ares VPS and do not imply actual VI latency.
 * stats may be NULL; renderer is diagnostic-only. */
void ck64_wireframe_draw(surface_t *screen, const CK64World3D *world,
                         unsigned long game_updates, unsigned grid_mode,
                         const CK64PerfTimes *previous,
                         CK64WireframeStats *stats);
#endif
