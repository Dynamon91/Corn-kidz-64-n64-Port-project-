#include "ck64_world3d.h"
#include <math.h>
#include <limits.h>
#include <stdint.h>

#define CK64_STEP (1.0f / 30.0f)
#define CK64_PI 3.14159265358979323846f

static float clampf(float x, float lo, float hi) {
    return x < lo ? lo : x > hi ? hi : x;
}
static float move_towards(float x, float target, float maximum) {
    if (x < target) return x + fminf(maximum, target - x);
    return x - fminf(maximum, x - target);
}
static float dot(CK64Vec3 a, CK64Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
static CK64Vec3 subtract(CK64Vec3 a, CK64Vec3 b) {
    CK64Vec3 v = {a.x-b.x, a.y-b.y, a.z-b.z}; return v;
}
static CK64Vec3 cross(CK64Vec3 a, CK64Vec3 b) {
    CK64Vec3 v = {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
    return v;
}
static CK64Vec3 normalized(CK64Vec3 v) {
    float d = sqrtf(dot(v,v));
    if (d < 0.00001f) { CK64Vec3 fallback={0.0f, 0.0f, 1.0f}; return fallback; }
    CK64Vec3 n={v.x/d, v.y/d, v.z/d}; return n;
}

/* Expanded AABB accounts for the width of the player proxy. */
static bool overlaps_pillar(const CK64World3D *w, float x, float z) {
    const float r = CK64_TEST_PILLAR_HALF + CK64_TEST_AVATAR_HALF;
    return w->position.y < CK64_TEST_PILLAR_HEIGHT &&
           w->position.y + CK64_TEST_AVATAR_HEIGHT > 0.0f &&
           x > CK64_TEST_PILLAR_X - r && x < CK64_TEST_PILLAR_X + r &&
           z > CK64_TEST_PILLAR_Z - r && z < CK64_TEST_PILLAR_Z + r;
}
bool ck64_world3d_set_collision_enabled(CK64World3D *w, bool enabled) {
    if (!w) return false;
    w->collision_enabled = enabled;
    /* Disabling collision also clears stale impact feedback; lifetime totals
     * remain until B/reset. A later contact starts a new event. */
    w->contact_last_step = false;
    w->collided_this_step = false;
    w->contact_flash_steps = 0;
    w->ejection_flash_steps = 0;
    if (!enabled || !overlaps_pillar(w, w->position.x, w->position.z))
        return true;
    const float r = CK64_TEST_PILLAR_HALF + CK64_TEST_AVATAR_HALF;
    const float left = CK64_TEST_PILLAR_X-r, right = CK64_TEST_PILLAR_X+r;
    const float front = CK64_TEST_PILLAR_Z-r, back = CK64_TEST_PILLAR_Z+r;
    const float d[4] = {w->position.x-left, right-w->position.x,
                        w->position.z-front, back-w->position.z};
    unsigned face = 0;
    for (unsigned i=1; i<4; ++i) if (d[i] < d[face]) face=i;
    if (face==0) {w->position.x=left;  w->velocity.x=0.0f;}
    if (face==1) {w->position.x=right; w->velocity.x=0.0f;}
    if (face==2) {w->position.z=front; w->velocity.z=0.0f;}
    if (face==3) {w->position.z=back;  w->velocity.z=0.0f;}
    w->ejection_flash_steps = 12; /* On-screen EJECTED for 0.4 s at 30 Hz. */
    return true;
}

void ck64_world3d_reset(CK64World3D *world) {
    if (!world) return;
    *world = (CK64World3D){0};
    world->grounded = true;
    world->collision_enabled = true;
}
bool ck64_world3d_step(CK64World3D *world, CK64WorldInput in) {
    if (!world || !isfinite(in.stick_x) || !isfinite(in.stick_y) ||
        !isfinite(in.camera_turn) || world->total_steps == UINT_MAX) return false;
    if (in.reset) { ck64_world3d_reset(world); return true; }
    if (world->contact_flash_steps > 0) --world->contact_flash_steps;
    if (world->ejection_flash_steps > 0) --world->ejection_flash_steps;
    world->collided_this_step = false;
    float sx = clampf(in.stick_x, -1.0f, 1.0f);
    float sy = clampf(in.stick_y, -1.0f, 1.0f);
    float mag = sqrtf(sx*sx+sy*sy);
    if (mag < 0.12f) { sx=0.0f; sy=0.0f; }
    else if (mag > 1.0f) { sx/=mag; sy/=mag; }

    world->camera_yaw += clampf(in.camera_turn,-1.0f,1.0f) * (1.8f*CK64_STEP);
    if (world->camera_yaw > CK64_PI) world->camera_yaw -= 2.0f*CK64_PI;
    if (world->camera_yaw < -CK64_PI) world->camera_yaw += 2.0f*CK64_PI;
    float sn = sinf(world->camera_yaw), cs = cosf(world->camera_yaw);
    float dx = (cs*sx + sn*sy) * 4.0f;
    float dz = (-sn*sx + cs*sy) * 4.0f;
    world->velocity.x = move_towards(world->velocity.x, dx, 18.0f*CK64_STEP);
    world->velocity.z = move_towards(world->velocity.z, dz, 18.0f*CK64_STEP);
    /* Axis-separated resolution preserves tangential sliding. At 4 units/s
     * each 30 Hz displacement is <=0.134, far less than pillar width 2.16;
     * the player cannot tunnel completely through this test-only obstacle. */
    float nx = world->position.x + world->velocity.x*CK64_STEP;
    if (nx <= -7.5f || nx >= 7.5f) {
        nx = clampf(nx,-7.5f,7.5f);
        world->velocity.x = 0.0f;
    }
    if (world->collision_enabled &&
        overlaps_pillar(world,nx,world->position.z)) {
        const float r = CK64_TEST_PILLAR_HALF + CK64_TEST_AVATAR_HALF;
        nx = CK64_TEST_PILLAR_X + (world->velocity.x>0.0f ? -r : r);
        world->velocity.x = 0.0f;
        world->collided_this_step = true;
    }
    world->position.x = nx;
    float nz = world->position.z + world->velocity.z*CK64_STEP;
    if (nz <= -7.5f || nz >= 7.5f) {
        nz = clampf(nz,-7.5f,7.5f);
        world->velocity.z = 0.0f;
    }
    if (world->collision_enabled &&
        overlaps_pillar(world,world->position.x,nz)) {
        const float r = CK64_TEST_PILLAR_HALF + CK64_TEST_AVATAR_HALF;
        nz = CK64_TEST_PILLAR_Z + (world->velocity.z>0.0f ? -r : r);
        world->velocity.z = 0.0f;
        world->collided_this_step = true;
    }
    world->position.z = nz;
    if (world->collided_this_step) {
        if (world->collision_hits < UINT_MAX) ++world->collision_hits;
        if (!world->contact_last_step) {
            if (world->contact_events < UINT_MAX) ++world->contact_events;
            world->contact_flash_steps = 10;
        }
    }
    world->contact_last_step = world->collided_this_step;
    if (in.jump && world->grounded) {
        world->grounded = false;
        world->velocity.y = 6.0f;
    }
    if (!world->grounded) {
        world->velocity.y -= 14.0f*CK64_STEP;
        world->position.y += world->velocity.y*CK64_STEP;
        if (world->position.y <= 0.0f) {
            world->position.y = 0.0f;
            world->velocity.y = 0.0f;
            world->grounded = true;
        }
    }
    ++world->total_steps;
    return true;
}
bool ck64_world3d_project(const CK64World3D *world, CK64Vec3 point,
                          CK64ScreenPoint *screen) {
    if (!world || !screen || !isfinite(point.x) ||
        !isfinite(point.y) || !isfinite(point.z)) return false;
    const float sn = sinf(world->camera_yaw), cs=cosf(world->camera_yaw);
    CK64Vec3 eye = {world->position.x - 7.0f*sn,
                  world->position.y + 4.0f,
                  world->position.z - 7.0f*cs};
    CK64Vec3 target = {world->position.x,world->position.y + 0.8f,world->position.z};
    CK64Vec3 forward = normalized(subtract(target,eye));
    CK64Vec3 y_axis = {0.0f,1.0f,0.0f};
    CK64Vec3 right = normalized(cross(y_axis,forward));
    CK64Vec3 up = cross(forward,right);
    CK64Vec3 view=subtract(point,eye);
    float depth = dot(view,forward);
    if (!isfinite(depth) || depth <= 0.35f) return false;
    CK64ScreenPoint out = {160.0f + 170.0f * dot(view,right)/depth,
                          120.0f - 170.0f * dot(view,up)/depth};
    if (!isfinite(out.x) || !isfinite(out.y) ||
         fabsf(out.x)>20000.0f || fabsf(out.y)>20000.0f) return false;
    *screen=out; return true;
}
/* Clip a world-space segment against the camera near plane BEFORE projection.
 * The legacy wireframe renderer rejected an entire segment whenever either
 * endpoint had depth <= 0.35f. This hid visible parts of the floor grid.
 * Preserve caller outputs if the line is not visible or an input is invalid.
 * The additional 0.01f guard is intentional: project() rejects <= 0.35f.
 */
bool ck64_world3d_project_line(const CK64World3D *world,
                               CK64Vec3 from, CK64Vec3 to,
                               CK64ScreenPoint *out_a, CK64ScreenPoint *out_b) {
    const float near_depth = 0.36f;
    if (!world || !out_a || !out_b ||
        !isfinite(world->camera_yaw) ||
        !isfinite(world->position.x) || !isfinite(world->position.y) ||
        !isfinite(world->position.z) ||
        !isfinite(from.x) || !isfinite(from.y) || !isfinite(from.z) ||
        !isfinite(to.x) || !isfinite(to.y) || !isfinite(to.z))
        return false;

    const float sn = sinf(world->camera_yaw), cs = cosf(world->camera_yaw);
    const CK64Vec3 eye = {
        world->position.x - 7.0f * sn,
        world->position.y + 4.0f,
        world->position.z - 7.0f * cs
    };
    const CK64Vec3 target = {
        world->position.x, world->position.y + 0.8f, world->position.z
    };
    const CK64Vec3 forward = normalized(subtract(target, eye));
    const float depth_a = dot(subtract(from, eye), forward);
    const float depth_b = dot(subtract(to, eye), forward);
    if (!isfinite(depth_a) || !isfinite(depth_b) ||
        (depth_a < near_depth && depth_b < near_depth))
        return false;

    if (depth_a < near_depth) {
        const float t = (near_depth - depth_a) / (depth_b - depth_a);
        from.x += t * (to.x - from.x);
        from.y += t * (to.y - from.y);
        from.z += t * (to.z - from.z);
    } else if (depth_b < near_depth) {
        const float t = (near_depth - depth_a) / (depth_b - depth_a);
        to.x = from.x + t * (to.x - from.x);
        to.y = from.y + t * (to.y - from.y);
        to.z = from.z + t * (to.z - from.z);
    }

    CK64ScreenPoint a, b;
    if (!ck64_world3d_project(world, from, &a) ||
        !ck64_world3d_project(world, to, &b) ||
        !ck64_world3d_clip_line(&a, &b))
        return false;
    *out_a = a;
    *out_b = b;
    return true;
}

/* Liang-Barsky clipping within inclusive 320x240 viewport. */
bool ck64_world3d_clip_line(CK64ScreenPoint *a, CK64ScreenPoint *b) {
    if (!a || !b || !isfinite(a->x) || !isfinite(a->y) ||
        !isfinite(b->x) || !isfinite(b->y)) return false;
    float dx=b->x-a->x,dy=b->y-a->y,lo=0.0f,hi=1.0f;
    const float p[4]={-dx,dx,-dy,dy};
    const float q[4]={a->x,319.0f-a->x,a->y,239.0f-a->y};
    for(int i=0;i<4;i++) {
        if (fabsf(p[i]) < 1.0e-7f) {
            if (q[i]<0.0f) return false;
        } else {
            float r=q[i]/p[i];
            if (p[i]<0.0f) { if (r>lo) lo=r; }
            else { if (r<hi) hi=r; }
            if (lo>hi) return false;
        }
    }
    CK64ScreenPoint out_a={a->x+lo*dx,a->y+lo*dy};
    CK64ScreenPoint out_b={a->x+hi*dx,a->y+hi*dy};
    *a=out_a; *b=out_b; return true;
}
