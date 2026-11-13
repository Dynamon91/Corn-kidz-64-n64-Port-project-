#ifndef CK64_WIREFRAME_H
#define CK64_WIREFRAME_H
/* Phase 8 diagnostic-only perspective wireframe, no original game graphics.
 * Uses libdragon's unaccelerated line primitives, not N64 3D RSP/RDP yet. */
#include <libdragon.h>
#include "ck64_world3d.h"
void ck64_wireframe_draw(surface_t *screen, const CK64World3D *world,
                          unsigned long game_updates);
#endif
