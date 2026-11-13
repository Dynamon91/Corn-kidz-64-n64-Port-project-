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
int main(void) {
    test_reset_and_standstill();test_move_and_bounds();
    test_jump_landing();test_steering_and_reset();
    test_invalid_inputs_atomic();test_perspective_projection();
    test_screen_clip();
    puts("7 wireframe/navigation prototype scenario tests passed");return 0;
}
