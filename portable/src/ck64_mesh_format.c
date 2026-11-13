#include "ck64_mesh_format.h"
#include <math.h>
#include <string.h>

static uint16_t read16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0]<<8)|(uint16_t)p[1]);
}
static uint32_t read32(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|
           ((uint32_t)p[2]<<8)|(uint32_t)p[3];
}
static float read_float(const uint8_t *p) {
    uint32_t bits=read32(p);
    float result;
    if (sizeof(float)!=sizeof(uint32_t)) return NAN;
    memcpy(&result,&bits,sizeof result);
    return result;
}
bool ck64_mesh_view_open(CK64MeshView *out, const uint8_t *data,size_t n) {
    if(!out||!data||n<28u||memcmp(data,"CKM1",4)!=0) return false;
    uint32_t nv=read32(data+4),nt=read32(data+8);
    if(nv==0u||nv>65535u||nt==0u||nt>65535u ||
       n!=28u+(size_t)6u*((size_t)nv+(size_t)nt))return false;
    CK64MeshView m={0};
    m.bytes=data;m.byte_length=n;m.vertex_count=nv;m.triangle_count=nt;
    for(int i=0;i<3;i++) {
        m.center[i]=read_float(data+12+4*i);
        if(!isfinite(m.center[i]))return false;
    }
    m.units_per_i16=read_float(data+24);
    if(!isfinite(m.units_per_i16)||m.units_per_i16<=0.0f)return false;
    size_t offset=28u+(size_t)nv*6u;
    for(uint32_t i=0;i<nt*3u;i++)
        if(read16(data+offset+(size_t)i*2u)>=nv)return false;
    *out=m;
    return true;
}
bool ck64_mesh_position(const CK64MeshView *m,uint32_t idx,
                        CK64MeshPosition *out) {
    if(!m||!m->bytes||!out||idx>=m->vertex_count ||
       m->byte_length!=28u+6u*((size_t)m->vertex_count+(size_t)m->triangle_count))
        return false;
    const uint8_t *p=m->bytes+28u+(size_t)idx*6u;
    CK64MeshPosition v={
        m->center[0]+(float)(int16_t)read16(p)  *m->units_per_i16,
        m->center[1]+(float)(int16_t)read16(p+2)*m->units_per_i16,
        m->center[2]+(float)(int16_t)read16(p+4)*m->units_per_i16
    };
    if(!isfinite(v.x)||!isfinite(v.y)||!isfinite(v.z))return false;
    *out=v;
    return true;
}
bool ck64_mesh_triangle(const CK64MeshView *m,uint32_t idx,
                        CK64MeshTriangle *out) {
    if(!m||!m->bytes||!out||idx>=m->triangle_count ||
       m->byte_length!=28u+6u*((size_t)m->vertex_count+(size_t)m->triangle_count))
        return false;
    const uint8_t *p=m->bytes+28u+(size_t)m->vertex_count*6u+(size_t)idx*6u;
    CK64MeshTriangle tri={read16(p),read16(p+2),read16(p+4)};
    if(tri.a>=m->vertex_count||tri.b>=m->vertex_count||tri.c>=m->vertex_count)
        return false;
    *out=tri;
    return true;
}
