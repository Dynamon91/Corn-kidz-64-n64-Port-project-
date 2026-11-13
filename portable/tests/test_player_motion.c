#include "ck64_player_motion.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int eq(float a,float b,float tol){return fabsf(a-b)<=tol;}
static CK64MoveParams args(int direct){
    CK64MoveParams p={{0,0,1},0.0f,0.1f,0.2f,direct!=0,false,2.0f};
    return p;
}
static void direct_accelerate(void){
    CK64PlayerMotion m;assert(ck64_player_motion_init(&m));
    m.ctrl_dir=(CK64PlayerVec3){0,0,0.8f};
    CK64MoveParams p=args(1);
    assert(ck64_player_motion_step(&m,p));
    assert(eq(m.ctrl_dir.z,1.0f,0.0001f));
    assert(eq(m.z_mov,0.1f,0.0001f));
    assert(eq(m.pl_vel.z,0.2f,0.0001f));
    assert(ck64_player_motion_step(&m,p));
    assert(eq(m.z_mov,0.2f,0.0001f));
}
static void direct_brake_lerp_clamped(void){
    CK64PlayerMotion m;assert(ck64_player_motion_init(&m));
    m.z_mov=1.0f;
    CK64MoveParams p=args(1);p.brake=0.25f;
    assert(ck64_player_motion_step(&m,p));
    assert(eq(m.z_mov,0.75f,0.0001f));
    assert(eq(m.pl_vel.z,1.5f,0.0001f));
    p.brake=3.0f;assert(ck64_player_motion_step(&m,p));
    assert(eq(m.z_mov,0.0f,0.0001f));
}
static void direct_threshold(void){
    CK64PlayerMotion m;assert(ck64_player_motion_init(&m));
    m.ctrl_dir=(CK64PlayerVec3){0,0,0.59f};
    assert(ck64_player_motion_step(&m,args(1)));
    assert(eq(m.ctrl_dir.z,0.59f,0.0001f));
    m.ctrl_dir.z=0.6f;assert(ck64_player_motion_step(&m,args(1)));
    assert(eq(m.ctrl_dir.z,1.0f,0.0001f));
}
static void inertia_accel_then_friction(void){
    CK64PlayerMotion m;assert(ck64_player_motion_init(&m));
    m.pl_vel=(CK64PlayerVec3){0,3,1};m.z_mov=0.5f;
    m.ctrl_dir=(CK64PlayerVec3){0,0,1};
    CK64MoveParams p=args(0);p.brake=0.05f;
    assert(ck64_player_motion_step(&m,p));
    assert(eq(m.pl_vel.y,0,0.0001f));
    assert(m.pl_vel.z>0 && m.pl_vel.z<=2);
    assert(eq(m.z_mov,m.pl_vel.z/2.0f,0.0001f));
}
static void inertia_to_zero(void){
    CK64PlayerMotion m;assert(ck64_player_motion_init(&m));
    m.pl_vel=(CK64PlayerVec3){0,0,1};m.z_mov=0.5f;
    CK64MoveParams p=args(0);p.brake=1.5f;
    assert(ck64_player_motion_step(&m,p));
    assert(eq(m.pl_vel.z,0,0.0001f)&&eq(m.z_mov,0,0.0001f));
}
static void inertia_speed_limit(void){
    CK64PlayerMotion m;assert(ck64_player_motion_init(&m));
    m.pl_vel=(CK64PlayerVec3){0,0,5};m.z_mov=3.0f;
    CK64MoveParams p=args(0);p.brake=0;
    assert(ck64_player_motion_step(&m,p));
    assert(eq(m.pl_vel.z,5.6f,0.0001f));
    assert(eq(m.z_mov,2.8f,0.0001f));
}
static void rotation_absolute_vs_fraction(void){
    CK64PlayerMotion m;assert(ck64_player_motion_init(&m));
    const float a=0.7853981633974483f;
    m.ctrl_rot=(CK64Quat){0,sinf(a),0,cosf(a)};
    m.ctrl_dir=(CK64PlayerVec3){0,0,1};
    CK64MoveParams p=args(1);p.turn_step=30.0f;
    assert(ck64_player_motion_step(&m,p));
    assert(eq(m.rotation.y,sinf(0.2617993878f),0.0002f));
    assert(ck64_player_motion_init(&m));
    m.ctrl_rot=(CK64Quat){0,sinf(a),0,cosf(a)};m.ctrl_dir.z=1;
    p.fractional_turn=true;p.turn_step=0.25f;
    assert(ck64_player_motion_step(&m,p));
    assert(eq(m.rotation.y,sinf(0.19634954f),0.0002f));
}
static void sloped_movement_and_upright(void){
    CK64PlayerMotion m;assert(ck64_player_motion_init(&m));
    const float s=0.707106781f;
    m.floor_norm=(CK64PlayerVec3){0,s,s};m.ctrl_dir.z=1;
    CK64MoveParams p=args(1);p.turn_step=0;
    assert(ck64_player_motion_step(&m,p));
    assert(eq(m.pl_vel.z,s*0.2f,0.0002f));
    assert(eq(m.pl_vel.y,-s*0.2f,0.0002f));
    assert(eq(m.rotation.x,0,0.0001f));
}
static void invalid_atomic(void){
    CK64PlayerMotion m;assert(ck64_player_motion_init(&m));
    CK64PlayerMotion before=m;CK64MoveParams p=args(1);
    assert(!ck64_player_motion_step(NULL,p));
    p.speed_scale=0;assert(!ck64_player_motion_step(&m,p));
    p=args(1);p.acceleration=NAN;
    assert(!ck64_player_motion_step(&m,p));
    p=args(1);p.movement.z=INFINITY;
    assert(!ck64_player_motion_step(&m,p));
    p=args(1);m.floor_norm=(CK64PlayerVec3){0,0,0};
    assert(!ck64_player_motion_step(&m,p));
    m=before;assert(memcmp(&m,&before,sizeof m)==0);
}
int main(void){
    direct_accelerate();direct_brake_lerp_clamped();direct_threshold();
    inertia_accel_then_friction();inertia_to_zero();inertia_speed_limit();
    rotation_absolute_vs_fraction();sloped_movement_and_upright();invalid_atomic();
    puts("9 source-grounded PlayerCtrl::MovPl scenario groups passed");
    return 0;
}
