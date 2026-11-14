#include "ck64_mesh_scene.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void trs_rotation_scale_translation(void) {
    CK64Affine3x4 m;
    const float p[3]={10,20,30},q[4]={0,0,0.70710678118f,0.70710678118f};
    const float s[3]={2,3,4};
    CK64MeshPosition out;
    assert(ck64_affine3x4_from_unity_trs(&m,p,q,s));
    assert(ck64_affine3x4_transform_point(&m,(CK64MeshPosition){1,0,0},&out));
    assert(fabsf(out.x-10)<0.0001f && fabsf(out.y-22)<0.0001f &&
           fabsf(out.z-30)<0.0001f);
}
static void parent_times_local(void) {
    CK64Affine3x4 p,l,w; CK64MeshPosition out;
    const float pp[3]={10,0,0},pq[4]={0,0,0,1},ps[3]={2,2,2};
    const float lp[3]={1,0,0},lq[4]={0,0,0,1},ls[3]={1,1,1};
    assert(ck64_affine3x4_from_unity_trs(&p,pp,pq,ps));
    assert(ck64_affine3x4_from_unity_trs(&l,lp,lq,ls));
    assert(ck64_affine3x4_multiply(&w,&p,&l));
    assert(ck64_affine3x4_transform_point(&w,(CK64MeshPosition){0,0,0},&out));
    assert(fabsf(out.x-12)<0.0001f);
}
static void invalid_trs_is_atomic(void) {
    CK64Affine3x4 m={{{1,2,3,4},{5,6,7,8},{9,10,11,12}}},before=m;
    const float p[3]={0,0,0},q[4]={0,0,0,0},s[3]={1,1,1};
    assert(!ck64_affine3x4_from_unity_trs(&m,p,q,s));
    assert(m.m[0][0]==before.m[0][0] && m.m[2][3]==before.m[2][3]);
}
static void invalid_multiply_is_atomic(void) {
    CK64Affine3x4 a={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}},out=a,b=a;
    b.m[1][2]=NAN;
    assert(!ck64_affine3x4_multiply(&out,&a,&b));
    assert(out.m[0][0]==1 && out.m[1][1]==1);
}
static void invalid_point_is_atomic(void) {
    CK64Affine3x4 m={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
    CK64MeshPosition out={7,8,9};
    assert(!ck64_affine3x4_transform_point(&m,(CK64MeshPosition){NAN,0,0},&out));
    assert(out.x==7 && out.y==8 && out.z==9);
}
static void triangle_cap(void) {
    uint8_t data[28+6*(3+513)]={0};
    CK64MeshView v={data,sizeof data,3,513,{0,0,0},1};
    assert(ck64_mesh_scene_validate(&v));
    assert(ck64_mesh_scene_triangle_count(&v,CK64_PHASE23_TRIANGLE_BUDGET)==256u);
    assert(ck64_mesh_scene_triangle_count(&v,600u)==513u);
    assert(ck64_mesh_scene_triangle_count(&v,0u)==0u);
}
static void reject_oversize_or_malformed(void) {
    CK64MeshView v={0};
    uint8_t data[52]={0};
    v=(CK64MeshView){data,sizeof data,3,1,{0,0,0},1};
    assert(ck64_mesh_scene_validate(&v));
    v.byte_length=CK64_PHASE23_MAX_PRIVATE_MESH_BYTES+1u;
    assert(!ck64_mesh_scene_validate(&v));
    v.byte_length=51u;
    assert(!ck64_mesh_scene_validate(&v));
}
static void memory_budget(void) {
    assert(CK64_PHASE23_FRAMEBUFFER_BYTES==307200u);
    assert(CK64_PHASE23_ZBUFFER_BYTES==153600u);
    assert(CK64_PHASE23_TEXTURE_TMEM_BYTES==0u);
    assert(ck64_mesh_scene_rdram_bytes(68590u)==530462u);
    assert(ck64_mesh_scene_rdram_bytes(CK64_PHASE23_MAX_PRIVATE_MESH_BYTES)==595968u);
    assert(ck64_mesh_scene_rdram_bytes(CK64_PHASE23_MAX_PRIVATE_MESH_BYTES+1u)==0u);
}
int main(void) {
    trs_rotation_scale_translation();parent_times_local();invalid_trs_is_atomic();
    invalid_multiply_is_atomic();invalid_point_is_atomic();triangle_cap();
    reject_oversize_or_malformed();memory_budget();
    puts("8 CK64 mesh scene scenario groups passed"); return 0;
}
