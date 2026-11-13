#include "ck64_world3d.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool near(float a,float b,float eps){ return fabsf(a-b)<eps; }
static void test_reset_and_standstill(void) {
    CK64World3D w;
    ck64_world3d_reset(&w);
    assert(w.grounded && w.position.y==0.0f);
    CK64WorldInput in={0};
    for(int i=0;i<100;i++) assert(ck64_world3d_step(&w,in));
    assert(w.total_steps==100 && near(w.position.x,0,0.0001f) && near(w.position.z,0,0.0001f));
}
static void test_move_and_bounds(void) {
    CK64World3D w;ck64_world3d_reset(&w);
    CK64WorldInput in={0};in.stick_y=1.0f;
    for(int i=0;i<200;i++) assert(ck64_world3d_step(&w,in));
    assert(near(w.position.z,7.5f,0.0001f) && w.velocity.z==0.0f);
    in.stick_y=-1.0f;
    for(int i=0;i<200;i++) assert(ck64_world3d_step(&w,in));
    assert(w.position.z < 0.0f && w.position.z >= -7.5f);
}
static void test_jump_landing(void) {
    CK64World3D w;ck64_world3d_reset(&w);
    CK64WorldInput in={0};in.jump=true;
    assert(ck64_world3d_step(&w,in));
    assert(w.position.y>0.0f && !w.grounded);
    float first_y=w.position.y;
    in.jump=false; /* jump is an edge event; holding would trigger a new jump */
    for(int i=0;i<70;i++) assert(ck64_world3d_step(&w,in));
    assert(w.position.y==0.0f && w.grounded && w.velocity.y==0.0f);
    assert(first_y>0.0f);
}
static void test_steering_and_reset(void) {
    CK64World3D w;ck64_world3d_reset(&w);
    CK64WorldInput in={0};in.camera_turn=1.0f;in.stick_y=1.0f;
    for(int i=0;i<20;i++) assert(ck64_world3d_step(&w,in));
    assert(w.camera_yaw>0.5f && w.position.x>0.0f);
    in.reset=true;assert(ck64_world3d_step(&w,in));
    assert(w.total_steps==0 && near(w.camera_yaw,0,0.001f));
}
static void test_invalid_inputs_atomic(void) {
    CK64World3D w;ck64_world3d_reset(&w);
    CK64WorldInput in={0};in.stick_x=NAN;
    CK64World3D before=w;
    assert(!ck64_world3d_step(&w,in));
    assert(memcmp(&w,&before,sizeof w)==0);
    assert(!ck64_world3d_step(NULL,(CK64WorldInput){0}));
}
static void test_perspective_projection(void) {
    CK64World3D w;ck64_world3d_reset(&w);
    CK64ScreenPoint c={0};
    assert(ck64_world3d_project(&w,(CK64Vec3){0,0.8f,0},&c));
    assert(near(c.x,160.0f,0.1f) && near(c.y,120.0f,0.1f));
    assert(!ck64_world3d_project(&w,(CK64Vec3){0,0,-20},&c));
    assert(!ck64_world3d_project(&w,(CK64Vec3){NAN,0,0},&c));
}
static void test_screen_clip(void) {
    CK64ScreenPoint a={-10,100},b={400,100};
    assert(ck64_world3d_clip_line(&a,&b));
    assert(near(a.x,0.0f,0.001f) && near(b.x,319.0f,0.001f));
    a=(CK64ScreenPoint){-10,-10}; b=(CK64ScreenPoint){-5,-5};
    assert(!ck64_world3d_clip_line(&a,&b));
    a=(CK64ScreenPoint){10,10};b=(CK64ScreenPoint){10,10};
    assert(ck64_world3d_clip_line(&a,&b));
    assert(near(a.x,10.0f,0.001f));
}
/* The camera follows the avatar. Test a line from exactly the camera eye to
 * the avatar chest: the old renderer rejected the entire segment because its
 * first endpoint could not be projected.
 */
static void test_line_crosses_near_plane(void) {
    const float yaw_values[] = {0.0f, 1.0f, -2.1f, 3.0f};
    for (unsigned i = 0; i < sizeof(yaw_values)/sizeof(yaw_values[0]); ++i) {
        CK64World3D w; ck64_world3d_reset(&w);
        w.camera_yaw = yaw_values[i];
        w.position.x = i % 2 ? 7.5f : -7.5f;
        w.position.z = i % 2 ? -7.5f : 7.5f;
        const CK64Vec3 eye = {
            w.position.x - 7.0f*sinf(w.camera_yaw),
            w.position.y + 4.0f,
            w.position.z - 7.0f*cosf(w.camera_yaw)
        };
        const CK64Vec3 center = {
            w.position.x, w.position.y + 0.8f, w.position.z
        };
        const CK64Vec3 behind = {
            eye.x - sinf(w.camera_yaw), eye.y, eye.z - cosf(w.camera_yaw)
        };
        CK64ScreenPoint a = {-5.0f,-5.0f}, b = {-6.0f,-6.0f};
        assert(!ck64_world3d_project(&w,eye,&a));
        assert(ck64_world3d_project_line(&w,eye,center,&a,&b));
        assert(near(a.x,160.0f,0.5f) && near(a.y,120.0f,0.5f));
        assert(near(b.x,160.0f,0.5f) && near(b.y,120.0f,0.5f));
        assert(ck64_world3d_project_line(&w,center,eye,&a,&b));
        assert(near(a.x,160.0f,0.5f) && near(b.x,160.0f,0.5f));
        assert(!ck64_world3d_project_line(&w,eye,behind,&a,&b));
    }
}
static void test_line_screen_clip_and_invalid_args(void) {
    CK64World3D w; ck64_world3d_reset(&w);
    CK64ScreenPoint a={-17,-18},b={-19,-20};
    const CK64Vec3 left={-12.0f,0.8f,0.0f};
    const CK64Vec3 right={12.0f,0.8f,0.0f};
    assert(ck64_world3d_project_line(&w,left,right,&a,&b));
    assert(near(a.x,0.0f,0.1f) && near(b.x,319.0f,0.1f));
    assert(near(a.y,120.0f,0.1f) && near(b.y,120.0f,0.1f));
    a=(CK64ScreenPoint){-17,-18}; b=(CK64ScreenPoint){-19,-20};
    assert(!ck64_world3d_project_line(&w,(CK64Vec3){NAN,0,0},
                                     right,&a,&b));
    assert(a.x==-17 && b.y==-20);
    assert(!ck64_world3d_project_line(NULL,left,right,&a,&b));
    assert(!ck64_world3d_project_line(&w,left,right,NULL,&b));
    assert(!ck64_world3d_project_line(&w,left,right,&a,NULL));
    assert(a.x==-17 && b.y==-20);
    w.camera_yaw=NAN;
    assert(!ck64_world3d_project_line(&w,left,right,&a,&b));
}
int main(void) {
    test_line_crosses_near_plane();test_line_screen_clip_and_invalid_args();
    test_reset_and_standstill();test_move_and_bounds();
    test_jump_landing();test_steering_and_reset();
    test_invalid_inputs_atomic();test_perspective_projection();
    test_screen_clip();
    puts("9 wireframe/navigation and near-plane scenario tests passed");return 0;
}
