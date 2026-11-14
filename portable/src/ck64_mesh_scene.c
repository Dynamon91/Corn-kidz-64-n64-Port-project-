#include "ck64_mesh_scene.h"
#include <math.h>
#include <stdint.h>

static bool finite3(const float v[3]) {
    return v && isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
}
static bool finite_matrix(const CK64Affine3x4 *m) {
    if (!m) return false;
    for (unsigned r=0; r<3; ++r)
        for (unsigned c=0; c<4; ++c)
            if (!isfinite(m->m[r][c])) return false;
    return true;
}
bool ck64_affine3x4_from_unity_trs(CK64Affine3x4 *out,
                                   const float p[3], const float q[4],
                                   const float s[3]) {
    if (!out || !finite3(p) || !finite3(s) || !q ||
        !isfinite(q[0]) || !isfinite(q[1]) ||
        !isfinite(q[2]) || !isfinite(q[3])) return false;
    const float norm2=q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3];
    if (!isfinite(norm2) || norm2 < 1e-12f) return false;
    const float inv=1.0f/sqrtf(norm2);
    const float x=q[0]*inv,y=q[1]*inv,z=q[2]*inv,w=q[3]*inv;
    CK64Affine3x4 m={{
        {(1.0f-2.0f*(y*y+z*z))*s[0], 2.0f*(x*y-z*w)*s[1],
         2.0f*(x*z+y*w)*s[2],p[0]},
        {2.0f*(x*y+z*w)*s[0],(1.0f-2.0f*(x*x+z*z))*s[1],
         2.0f*(y*z-x*w)*s[2],p[1]},
        {2.0f*(x*z-y*w)*s[0],2.0f*(y*z+x*w)*s[1],
         (1.0f-2.0f*(x*x+y*y))*s[2],p[2]}
    }};
    if (!finite_matrix(&m)) return false;
    *out=m;
    return true;
}
bool ck64_affine3x4_multiply(CK64Affine3x4 *out,
                             const CK64Affine3x4 *a,
                             const CK64Affine3x4 *b) {
    if (!out || !finite_matrix(a) || !finite_matrix(b)) return false;
    CK64Affine3x4 m={0};
    for (unsigned r=0;r<3;++r) {
        for (unsigned c=0;c<3;++c)
            for (unsigned k=0;k<3;++k) m.m[r][c]+=a->m[r][k]*b->m[k][c];
        m.m[r][3]=a->m[r][3];
        for (unsigned k=0;k<3;++k) m.m[r][3]+=a->m[r][k]*b->m[k][3];
    }
    if (!finite_matrix(&m)) return false;
    *out=m;
    return true;
}
bool ck64_affine3x4_transform_point(const CK64Affine3x4 *m,
                                    CK64MeshPosition p,
                                    CK64MeshPosition *out) {
    if (!out || !finite_matrix(m) || !isfinite(p.x) ||
        !isfinite(p.y) || !isfinite(p.z)) return false;
    CK64MeshPosition q={
        m->m[0][0]*p.x+m->m[0][1]*p.y+m->m[0][2]*p.z+m->m[0][3],
        m->m[1][0]*p.x+m->m[1][1]*p.y+m->m[1][2]*p.z+m->m[1][3],
        m->m[2][0]*p.x+m->m[2][1]*p.y+m->m[2][2]*p.z+m->m[2][3]
    };
    if (!isfinite(q.x)||!isfinite(q.y)||!isfinite(q.z)) return false;
    *out=q;
    return true;
}
bool ck64_mesh_scene_validate(const CK64MeshView *v) {
    return v && v->bytes && v->vertex_count>0u && v->triangle_count>0u &&
           v->byte_length<=CK64_PHASE23_MAX_PRIVATE_MESH_BYTES &&
           v->byte_length==28u+6u*((size_t)v->vertex_count+(size_t)v->triangle_count);
}
unsigned ck64_mesh_scene_triangle_count(const CK64MeshView *v,unsigned limit) {
    if (!ck64_mesh_scene_validate(v) || limit==0u) return 0u;
    return v->triangle_count<limit ? v->triangle_count : limit;
}
size_t ck64_mesh_scene_rdram_bytes(size_t mesh_bytes) {
    if (mesh_bytes>CK64_PHASE23_MAX_PRIVATE_MESH_BYTES) return 0u;
    return (size_t)CK64_PHASE23_FRAMEBUFFER_BYTES+
           (size_t)CK64_PHASE23_ZBUFFER_BYTES+mesh_bytes+
           (size_t)CK64_PHASE23_RENDER_SCRATCH_BYTES;
}
