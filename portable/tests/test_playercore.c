#include "ck64_playercore.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int eq(float a,float b) { return fabsf(a-b)<0.00001f; }
static void test_init(void) {
    CK64PlayerCore p; memset(&p,0xa5,sizeof p);
    assert(ck64_playercore_init(&p) && p.camera_type==1);
    assert(p.pl_state==0 && !p.on_floor && !p.no_attack);
    assert(!ck64_playercore_init(NULL));
}
static void test_gravity_default(void) {
    CK64PlayerCore p={0};
    assert(ck64_playercore_gravity(&p,0.02f,0.6f));
    assert(eq(p.y_velocity,-0.02f));
    for (int i=0;i<99;i++) assert(ck64_playercore_gravity(&p,0.02f,0.6f));
    assert(eq(p.y_velocity,-0.6f));
}
static void test_gravity_alternates(void) {
    CK64PlayerCore p={0}; p.y_velocity=0.4f;
    assert(ck64_playercore_gravity(&p,0.012f,0.6f) && eq(p.y_velocity,0.388f));
    assert(ck64_playercore_gravity(&p,0.003f,0.6f) && eq(p.y_velocity,0.385f));
    assert(ck64_playercore_gravity(&p,0.010f,0.6f) && eq(p.y_velocity,0.375f));
    assert(ck64_playercore_gravity(&p,0.015f,0.6f) && eq(p.y_velocity,0.360f));
    assert(ck64_playercore_gravity(&p,0.030f,0.6f) && eq(p.y_velocity,0.330f));
}
static void test_floor_override(void) {
    CK64PlayerCore p={0}; p.y_velocity=0.55f; p.on_floor=true;
    assert(ck64_playercore_gravity(&p,0.02f,0.6f) && eq(p.y_velocity,-0.1f));
    p.y_velocity=-3.0f;
    assert(ck64_playercore_gravity(&p,0.02f,0.6f) && eq(p.y_velocity,-0.1f));
}
static void test_selective_reset(void) {
    CK64PlayerCore p={0};
    p.pl_state=34; p.no_control=true; p.camera_type=17; p.z_mov=1.75f;
    p.pl_velocity=(CK64PlayerVec3){1,2,3};
    p.y_velocity=0.4f; p.on_floor=true; p.no_look=true; p.no_attack=true;
    p.camera_shake_ud=0.7f; p.fall_respawn_time=99.0f;
    assert(ck64_playercore_reset(&p));
    assert(p.pl_state==0 && !p.no_control && p.camera_type==1);
    assert(eq(p.z_mov,0) && eq(p.pl_velocity.x,0) &&
           eq(p.pl_velocity.y,0) && eq(p.pl_velocity.z,0));
    assert(eq(p.y_velocity,0.4f) && p.on_floor && p.no_look && p.no_attack);
    assert(eq(p.camera_shake_ud,0.7f) && eq(p.fall_respawn_time,99.0f));
    assert(!ck64_playercore_reset(NULL));
}
static void test_zxpos(void) {
    CK64PlayerVec3 v=ck64_playercore_zxpos((CK64PlayerVec3){5,12,-8});
    assert(eq(v.x,5) && eq(v.y,0) && eq(v.z,-8));
}
static void test_look_attack(void) {
    CK64PlayerCore p={0};
    assert(ck64_playercore_look_disable(&p) && p.no_look);
    assert(ck64_playercore_look_enable(&p) && !p.no_look);
    assert(ck64_playercore_attack_disable(&p) && p.no_attack);
    assert(!ck64_playercore_look_enable(NULL));
    assert(!ck64_playercore_look_disable(NULL));
    assert(!ck64_playercore_attack_disable(NULL));
}
static void test_shake_fallzone(void) {
    CK64PlayerCore p={0};
    assert(ck64_playercore_camera_shake(&p,1.5f));
    assert(ck64_playercore_set_fall_respawn(&p,3.2f));
    assert(eq(p.camera_shake_ud,1.5f) && eq(p.fall_respawn_time,3.2f));
    assert(!ck64_playercore_camera_shake(&p,NAN));
    assert(!ck64_playercore_set_fall_respawn(&p,INFINITY));
    assert(eq(p.camera_shake_ud,1.5f) && eq(p.fall_respawn_time,3.2f));
}
static void test_invalid_gravity(void) {
    CK64PlayerCore p={0}; p.y_velocity=0.5f;
    assert(!ck64_playercore_gravity(&p,NAN,0.6f));
    assert(!ck64_playercore_gravity(&p,0.02f,INFINITY));
    assert(!ck64_playercore_gravity(&p,INFINITY,0.6f));
    assert(!ck64_playercore_gravity(NULL,0.02f,0.6f));
    assert(eq(p.y_velocity,0.5f));
}
int main(void) {
    test_init(); test_gravity_default(); test_gravity_alternates();
    test_floor_override(); test_selective_reset(); test_zxpos();
    test_look_attack(); test_shake_fallzone(); test_invalid_gravity();
    puts("9 source-grounded PlayerCtrl core scenario groups passed");
    return 0;
}
