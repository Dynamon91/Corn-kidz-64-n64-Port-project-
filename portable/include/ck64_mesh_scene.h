#ifndef CK64_MESH_SCENE_H
#define CK64_MESH_SCENE_H
/* Phase 23: small, bounded Unity-TRS to CKM1 scene bridge.
 * Coordinates are Unity world units. This module carries no proprietary data.
 */
#include "ck64_mesh_format.h"
#include <stdbool.h>
#include <stddef.h>

#define CK64_PHASE23_TRIANGLE_BUDGET 256u
#define CK64_PHASE23_MAX_PRIVATE_MESH_BYTES 131072u
#define CK64_PHASE23_FRAMEBUFFER_BYTES (320u * 240u * 2u * 2u)
#define CK64_PHASE23_ZBUFFER_BYTES (320u * 240u * 2u)
#define CK64_PHASE23_RENDER_SCRATCH_BYTES 4096u
#define CK64_PHASE23_MAX_RDRAM_BYTES \
    (CK64_PHASE23_FRAMEBUFFER_BYTES + CK64_PHASE23_ZBUFFER_BYTES + \
     CK64_PHASE23_MAX_PRIVATE_MESH_BYTES + CK64_PHASE23_RENDER_SCRATCH_BYTES)
#define CK64_PHASE23_TEXTURE_TMEM_BYTES 0u

typedef struct CK64Affine3x4 {
    float m[3][4];
} CK64Affine3x4;

/* Unity quaternion is x,y,z,w. Each operation validates finite values and
 * leaves out untouched on failure. Matrices use row-major R*S plus translation.
 */
bool ck64_affine3x4_from_unity_trs(CK64Affine3x4 *out,
                                   const float position[3],
                                   const float rotation_xyzw[4],
                                   const float scale[3]);
bool ck64_affine3x4_multiply(CK64Affine3x4 *out,
                             const CK64Affine3x4 *parent,
                             const CK64Affine3x4 *local);
bool ck64_affine3x4_transform_point(const CK64Affine3x4 *matrix,
                                    CK64MeshPosition point,
                                    CK64MeshPosition *out);

/* Enforce the private asset and per-frame triangle limits before RDP work.
 * Returns zero when the view is invalid or the cap is zero.
 */
bool ck64_mesh_scene_validate(const CK64MeshView *view);
unsigned ck64_mesh_scene_triangle_count(const CK64MeshView *view,
                                        unsigned requested_limit);
/* Conservative renderer-owned RDRAM estimate, including two color buffers,
 * one 16-bit depth buffer, mesh bytes and bounded CPU scratch. Returns 0 if
 * mesh_bytes exceeds the configured cap.
 */
size_t ck64_mesh_scene_rdram_bytes(size_t mesh_bytes);
#endif
