#include "mesh_renderer.h"
#include <math.h>
#include <rdpq_attach.h>
#include <rdpq_mode.h>
#include <rdpq_tri.h>
#include <surface.h>

#define CK64_SCREEN_W 320.0f
#define CK64_SCREEN_H 240.0f
#define CK64_NEAR_SCALE 0.01f
#define CK64_FAR_SCALE 8.0f
#define CK64_CAMERA_SCALE 3.0f

typedef struct Point3 { float x,y,z; } Point3;
static surface_t zbuffer;

static Point3 sub(Point3 a,Point3 b) {
    return (Point3){a.x-b.x,a.y-b.y,a.z-b.z};
}
static Point3 add(Point3 a,Point3 b) {
    return (Point3){a.x+b.x,a.y+b.y,a.z+b.z};
}
static Point3 scale(Point3 a,float s) {
    return (Point3){a.x*s,a.y*s,a.z*s};
}
static float dot(Point3 a,Point3 b) {
    return a.x*b.x+a.y*b.y+a.z*b.z;
}
static Point3 cross(Point3 a,Point3 b) {
    return (Point3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
static bool normalize(Point3 *v) {
    const float n2=dot(*v,*v);
    if (!isfinite(n2)||n2<1e-12f) return false;
    *v=scale(*v,1.0f/sqrtf(n2));
    return isfinite(v->x)&&isfinite(v->y)&&isfinite(v->z);
}
static Point3 to_camera(Point3 p,Point3 eye,Point3 right,Point3 up,Point3 forward) {
    Point3 d=sub(p,eye);
    return (Point3){dot(d,right),dot(d,up),dot(d,forward)};
}
static unsigned clip_depth(const Point3 *in,unsigned n,Point3 *out,
                           float plane,bool keep_greater) {
    if (!n) return 0;
    unsigned count=0;
    Point3 a=in[n-1];
    bool a_in=keep_greater ? a.z>=plane : a.z<=plane;
    for (unsigned i=0;i<n;++i) {
        Point3 b=in[i];
        bool b_in=keep_greater ? b.z>=plane : b.z<=plane;
        if (a_in!=b_in) {
            const float den=b.z-a.z;
            if (fabsf(den)>1e-12f && count<6u) {
                const float t=(plane-a.z)/den;
                out[count++]=(Point3){a.x+(b.x-a.x)*t,
                                     a.y+(b.y-a.y)*t,plane};
            }
        }
        if (b_in && count<6u) out[count++]=b;
        a=b;a_in=b_in;
    }
    return count;
}
static bool project(Point3 p,float nearz,float farz,float *v) {
    if (!isfinite(p.x)||!isfinite(p.y)||!isfinite(p.z)||
        p.z<nearz||p.z>farz) return false;
    const float focal=160.0f;
    v[0]=CK64_SCREEN_W*0.5f+focal*p.x/p.z;
    v[1]=CK64_SCREEN_H*0.5f-focal*p.y/p.z;
    v[2]=(p.z-nearz)/(farz-nearz);
    return isfinite(v[0])&&isfinite(v[1])&&isfinite(v[2]);
}
static float area2(const float *a,const float *b,const float *c) {
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
}

bool ck64_n64_mesh_renderer_draw(surface_t *screen,const CK64MeshView *mesh,
                                 const CK64Affine3x4 *world,
                                 const float focus[3],float radius,
                                 float yaw,float pitch,CK64N64MeshStats *stats) {
    if (!screen||!mesh||!world||!focus||!stats||
        !ck64_mesh_scene_validate(mesh)||!isfinite(radius)||radius<=0.0f||
        !isfinite(yaw)||!isfinite(pitch)) return false;
    const size_t estimate=ck64_mesh_scene_rdram_bytes(mesh->byte_length);
    if (!estimate) return false;
    if (!zbuffer.buffer) {
        zbuffer=surface_alloc(FMT_RGBA16,320,240);
        if (!zbuffer.buffer) return false;
    }
    *stats=(CK64N64MeshStats){0,0,0,estimate};

    const float cp=cosf(pitch),sp=sinf(pitch);
    Point3 target={focus[0],focus[1],focus[2]};
    Point3 direction={sinf(yaw)*cp,sp,cosf(yaw)*cp};
    Point3 eye=add(target,scale(direction,radius*CK64_CAMERA_SCALE));
    Point3 forward=sub(target,eye);
    if (!normalize(&forward)) return false;
    Point3 right=cross(forward,(Point3){0,1,0});
    if (!normalize(&right)) return false;
    Point3 up=cross(right,forward);
    if (!normalize(&up)) return false;
    const float nearz=radius*CK64_NEAR_SCALE;
    const float farz=radius*CK64_FAR_SCALE;

    rdpq_attach_clear(screen,&zbuffer);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
    rdpq_mode_zbuf(true,true);
    const unsigned source_limit=ck64_mesh_scene_triangle_count(
        mesh,CK64_PHASE23_TRIANGLE_BUDGET);
    for (unsigned sample=0;sample<source_limit &&
         stats->rdp_triangles_submitted<CK64_PHASE23_TRIANGLE_BUDGET;++sample) {
        const uint32_t source=(uint32_t)(((uint64_t)sample*mesh->triangle_count)/
                                         source_limit);
        CK64MeshTriangle tri;
        CK64MeshPosition local[3],placed[3];
        Point3 poly[6],near_poly[6],clipped[6];
        if (!ck64_mesh_triangle(mesh,source,&tri)||
            !ck64_mesh_position(mesh,tri.a,&local[0])||
            !ck64_mesh_position(mesh,tri.b,&local[1])||
            !ck64_mesh_position(mesh,tri.c,&local[2])) {
            ++stats->clipped_or_degenerate;
            continue;
        }
        bool valid=true;
        for (unsigned k=0;k<3;++k) {
            if (!ck64_affine3x4_transform_point(world,local[k],&placed[k])) {
                valid=false;break;
            }
        }
        if (!valid) {++stats->clipped_or_degenerate;continue;}
        ++stats->source_triangles_sampled;
        for (unsigned k=0;k<3;++k) {
            Point3 p={placed[k].x,placed[k].y,placed[k].z};
            poly[k]=to_camera(p,eye,right,up,forward);
        }
        unsigned n=clip_depth(poly,3,near_poly,nearz,true);
        n=clip_depth(near_poly,n,clipped,farz,false);
        if (n<3u) {++stats->clipped_or_degenerate;continue;}
        float projected[6][3];
        bool pvalid=true;
        for (unsigned k=0;k<n;++k)
            if (!project(clipped[k],nearz,farz,projected[k])) pvalid=false;
        if (!pvalid) {++stats->clipped_or_degenerate;continue;}
        Point3 e1=sub((Point3){placed[1].x,placed[1].y,placed[1].z},
                      (Point3){placed[0].x,placed[0].y,placed[0].z});
        Point3 e2=sub((Point3){placed[2].x,placed[2].y,placed[2].z},
                      (Point3){placed[0].x,placed[0].y,placed[0].z});
        Point3 normal=cross(e1,e2);
        const float normal_len=sqrtf(dot(normal,normal));
        if (!isfinite(normal_len)||normal_len<1e-8f) {
            ++stats->clipped_or_degenerate;continue;
        }
        const float shade=0.30f+0.65f*fabsf(normal.y/normal_len);
        for (unsigned k=1;k+1<n &&
             stats->rdp_triangles_submitted<CK64_PHASE23_TRIANGLE_BUDGET;++k) {
            float *a=projected[0],*b=projected[k],*c=projected[k+1];
            if (fabsf(area2(a,b,c))<0.01f) {
                ++stats->clipped_or_degenerate;continue;
            }
            float v0[7]={a[0],a[1],a[2],shade,shade,shade,1.0f};
            float v1[7]={b[0],b[1],b[2],shade,shade,shade,1.0f};
            float v2[7]={c[0],c[1],c[2],shade,shade,shade,1.0f};
            rdpq_triangle(&TRIFMT_ZBUF_SHADE,v0,v1,v2);
            ++stats->rdp_triangles_submitted;
        }
    }
    rdpq_detach_wait();
    return true;
}
