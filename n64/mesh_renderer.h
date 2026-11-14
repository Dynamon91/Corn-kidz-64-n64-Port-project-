#ifndef CK64_MESH_RENDERER_H
#define CK64_MESH_RENDERER_H
/* Phase 23 RDP renderer: CPU TRS/projection, hardware RDP triangles. */
#include <libdragon.h>
#include "ck64_mesh_scene.h"

typedef struct CK64N64MeshStats {
    unsigned source_triangles_sampled;
    unsigned rdp_triangles_submitted;
    unsigned clipped_or_degenerate;
    size_t estimated_rdram_bytes;
} CK64N64MeshStats;

bool ck64_n64_mesh_renderer_draw(surface_t *screen, const CK64MeshView *mesh,
                                 const CK64Affine3x4 *world,
                                 const float focus[3], float radius,
                                 float yaw, float pitch,
                                 CK64N64MeshStats *stats);
#endif
