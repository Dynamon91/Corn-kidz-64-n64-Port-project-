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

static bool inside_expanded_pillar(const CK64World3D *w) {
    const float r=CK64_TEST_PILLAR_HALF+CK64_TEST_AVATAR_HALF;
    return w->position.x>CK64_TEST_PILLAR_X-r &&
           w->position.x<CK64_TEST_PILLAR_X+r &&
           w->position.z>CK64_TEST_PILLAR_Z-r &&
           w->position.z<CK64_TEST_PILLAR_Z+r &&
           w->position.y<CK64_TEST_PILLAR_HEIGHT;
}
static void test_solid_pillar_front_and_back(void) {
    CK64World3D w; ck64_world3d_reset(&w);
    const float r=CK64_TEST_PILLAR_HALF+CK64_TEST_AVATAR_HALF;
    w.position.x=CK64_TEST_PILLAR_X;
    CK64WorldInput in={0}; in.stick_y=1.0f;
    for (int i=0;i<150;i++) {
        assert(ck64_world3d_step(&w,in));
        assert(!inside_expanded_pillar(&w));
    }
    assert(w.collision_hits>0 && w.position.z<=CK64_TEST_PILLAR_Z-r+0.001f);
    ck64_world3d_reset(&w);
    w.position.x=CK64_TEST_PILLAR_X; w.position.z=7.0f;
    in.stick_y=-1.0f;
    for (int i=0;i<150;i++) assert(ck64_world3d_step(&w,in));
    assert(w.collision_hits>0 && w.position.z>=CK64_TEST_PILLAR_Z+r-0.001f);
}
static void test_diagonal_wall_slide(void) {
    CK64World3D w; ck64_world3d_reset(&w);
    w.position.x=1.8f; w.position.z=3.5f;
    CK64WorldInput in={0}; in.stick_x=1.0f; in.stick_y=1.0f;
    for (int i=0;i<100;i++) {
        assert(ck64_world3d_step(&w,in));
        assert(!inside_expanded_pillar(&w));
    }
    assert(w.collision_hits>0);
    assert(w.position.z>5.3f); /* Tangential movement is not blocked. */
}
static void test_toggle_and_reset_collision(void) {
    CK64World3D w; ck64_world3d_reset(&w);
    assert(w.collision_enabled);
    assert(!ck64_world3d_set_collision_enabled(NULL,false));
    assert(ck64_world3d_set_collision_enabled(&w,false));
    w.position.x=CK64_TEST_PILLAR_X;
    CK64WorldInput in={0}; in.stick_y=1.0f;
    for (int i=0;i<30;i++) assert(ck64_world3d_step(&w,in));
    assert(inside_expanded_pillar(&w) && w.collision_hits==0);
    assert(ck64_world3d_set_collision_enabled(&w,true));
    assert(!inside_expanded_pillar(&w));
    in.reset=true;
    assert(ck64_world3d_step(&w,in));
    assert(w.collision_enabled && w.collision_hits==0);
    assert(w.position.x==0.0f && w.position.z==0.0f);
}
static void test_airborne_pillar_collision(void) {
    CK64World3D w; ck64_world3d_reset(&w);
    w.position.x=CK64_TEST_PILLAR_X;
    w.position.z=2.8f;
    w.position.y=1.2f; w.grounded=false;
    CK64WorldInput in={0}; in.stick_y=1.0f;
    for (int i=0;i<16;i++) {
        assert(ck64_world3d_step(&w,in));
        assert(!inside_expanded_pillar(&w));
    }
    assert(w.collision_hits>0 && w.position.z<=2.921f);
}


static void test_distinct_contact_events(void) {
    CK64World3D w; ck64_world3d_reset(&w);
    w.position.x=CK64_TEST_PILLAR_X;
    w.position.z=2.5f;
    CK64WorldInput in={0}; in.stick_y=1.0f;
    for (int i=0; i<45; ++i) assert(ck64_world3d_step(&w,in));
    assert(w.collided_this_step);
    assert(w.collision_hits>10);
    assert(w.contact_events==1); /* Holding against wall is one impact. */

    /* Separate, leave the wall, and let the ten-step contact banner expire. */
    in.stick_y=-1.0f;
    for (int i=0; i<25; ++i) assert(ck64_world3d_step(&w,in));
    assert(!w.collided_this_step && !w.contact_last_step);
    assert(w.contact_flash_steps==0);
    assert(w.contact_events==1);
    in.stick_y=1.0f;
    for (int i=0; i<35; ++i) assert(ck64_world3d_step(&w,in));
    assert(w.collided_this_step && w.contact_events==2);
}
static void test_contact_flash_and_ejection_indicator(void) {
    CK64World3D w; ck64_world3d_reset(&w);
    w.position.x=CK64_TEST_PILLAR_X;
    w.position.z=2.85f;
    CK64WorldInput in={0}; in.stick_y=1.0f;
    assert(ck64_world3d_step(&w,in));
    assert(w.collided_this_step && w.contact_flash_steps==10);
    in.stick_y=-1.0f;
    assert(ck64_world3d_step(&w,in));
    assert(!w.collided_this_step && w.contact_flash_steps==9);
    for (int i=0;i<9;i++) assert(ck64_world3d_step(&w,in));
    assert(w.contact_flash_steps==0);
    assert(ck64_world3d_set_collision_enabled(&w,false));
    assert(w.contact_flash_steps==0 && !w.contact_last_step);
    w.position.x=CK64_TEST_PILLAR_X; w.position.z=CK64_TEST_PILLAR_Z;
    assert(ck64_world3d_set_collision_enabled(&w,true));
    assert(!inside_expanded_pillar(&w));
    assert(w.ejection_flash_steps==12 && w.contact_events==1);
    CK64WorldInput neutral={0};
    for (int i=0;i<12;i++) assert(ck64_world3d_step(&w,neutral));
    assert(w.ejection_flash_steps==0);
    assert(ck64_world3d_set_collision_enabled(&w,false));
    assert(w.ejection_flash_steps==0);
    neutral.reset=true; assert(ck64_world3d_step(&w,neutral));
    assert(w.contact_events==0 && w.collision_hits==0 &&
           w.contact_flash_steps==0 && w.ejection_flash_steps==0 &&
           w.collision_enabled);
}
static void test_camera_rotated_corner_slide(void) {
    CK64World3D w; ck64_world3d_reset(&w);
    w.position.x=1.7f; w.position.z=3.4f;
    w.camera_yaw=0.9f;
    CK64WorldInput in={0};
    /* Transform desired diagonal world movement into stick coordinates. */
    const float dx=0.70710678f, dz=0.70710678f;
    in.stick_x=cosf(w.camera_yaw)*dx - sinf(w.camera_yaw)*dz;
    in.stick_y=sinf(w.camera_yaw)*dx + cosf(w.camera_yaw)*dz;
    for (int i=0;i<80;i++) {
        assert(ck64_world3d_step(&w,in));
        assert(!inside_expanded_pillar(&w));
    }
    assert(w.collision_hits>0 && w.contact_events>=1);
    assert(w.position.z>5.1f && w.position.x>1.9f);
}

int main(void) {
    test_solid_pillar_front_and_back();
    test_diagonal_wall_slide();
    test_distinct_contact_events();
    test_contact_flash_and_ejection_indicator();
    test_camera_rotated_corner_slide();
    test_toggle_and_reset_collision();
    test_airborne_pillar_collision();
    test_line_crosses_near_plane();test_line_screen_clip_and_invalid_args();
    test_reset_and_standstill();test_move_and_bounds();
    test_jump_landing();test_steering_and_reset();
    test_invalid_inputs_atomic();test_perspective_projection();
    test_screen_clip();
    puts("16 navigation, projection, contact-event and collision scenario tests passed");return 0;
}
