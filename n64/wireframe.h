#ifndef CK64_WIREFRAME_H
#define CK64_WIREFRAME_H
/* Phase 12 N64 CPU wireframe diagnostics. No original game graphics.
 * Libdragon legacy line primitives are not a hardware 3D backend.
 */
#include <libdragon.h>
#include "ck64_world3d.h"

/* Order matches the controller R cycle: FULL -> SPARSE -> OFF -> FULL. */
enum { CK64_GRID_OFF = 0, CK64_GRID_SPARSE = 1, CK64_GRID_FULL = 2 };

typedef struct CK64WireframeStats {
    unsigned lines_attempted; /* World-space lines submitted for projection. */
    unsigned lines_drawn;     /* 2D lines surviving projection and clipping. */
} CK64WireframeStats;

/* Takes previous frame's drawing duration in microseconds. This is CPU time
 * inside the draw function; it EXCLUDES display_get/display_show wait and is
 * NOT the emulator's VPS or the N64's RDP workload. stats may be NULL.
 * This interface is experimental and does not import the original 3D assets.
 */
void ck64_wireframe_draw(surface_t *screen, const CK64World3D *world,
                         unsigned long game_updates,
                         unsigned grid_mode, unsigned previous_draw_us,
                         CK64WireframeStats *stats);
#endif
