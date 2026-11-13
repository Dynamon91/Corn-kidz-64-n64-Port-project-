#include "ck64_source_lab.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int eq(float a,float b){return fabsf(a-b)<0.0002f;}
static CK64SourceLab fresh(void){CK64SourceLab a;assert(ck64_source_lab_init(&a));return a;}
static void initialized(void){CK64SourceLab a=fresh();assert(a.initialized&&a.world.grounded);assert(a.floor_history.on_floor_timer==1);assert(!ck64_source_lab_init(NULL));}
static void idle(void){CK64SourceLab a=fresh();for(int i=0;i<20;i++)assert(ck64_source_lab_step(&a,(CK64SourceLabInput){0}));assert(eq(a.world.position.y,0)&&a.world.grounded);assert(a.floor_history.on_floor_timer==21);}
static void actual_horizontal(void){CK64SourceLab a=fresh();for(int i=0;i<30;i++)assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.stick_y=1}));assert(a.world.position.z>0.01f);assert(a.motion.z_mov>0);assert(a.world.total_steps==30);}
static void friction_stop(void){CK64SourceLab a=fresh();for(int i=0;i<20;i++)assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.stick_y=1}));float z=a.world.position.z;for(int i=0;i<50;i++)assert(ck64_source_lab_step(&a,(CK64SourceLabInput){0}));assert(a.world.position.z>z);assert(a.motion.z_mov<.2f);}
static void jump_and_land(void){CK64SourceLab a=fresh();assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.jump_press=true}));assert(a.jumps==1&&!a.world.grounded&&a.world.position.y>.20f);int landed=0;for(int i=0;i<60;i++){assert(ck64_source_lab_step(&a,(CK64SourceLabInput){0}));if(a.world.grounded){landed=1;break;}}assert(landed&&a.landings==1&&a.floor_history.on_floor_timer==1);}
static void early_release(void){CK64SourceLab a=fresh();assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.jump_press=true}));assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.jump_release=true}));assert(eq(a.state0.y_vel,.10f));}
static void camera_relative(void){CK64SourceLab a=fresh();for(int i=0;i<30;i++)assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.camera_turn=1}));assert(a.world.camera_yaw>1.7f);for(int i=0;i<20;i++)assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.stick_y=1}));assert(a.world.position.x>0.01f);}
static void pillar_collision(void){CK64SourceLab a=fresh();a.world.position.z=3.0f;a.world.position.x=0.0f;for(int i=0;i<150;i++)assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.stick_x=1}));assert(a.world.position.x<=CK64_TEST_PILLAR_X-CK64_TEST_PILLAR_HALF-CK64_TEST_AVATAR_HALF+0.0001f);assert(a.world.collision_hits>0);}
static void reset(void){CK64SourceLab a=fresh();assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.jump_press=true}));assert(ck64_source_lab_step(&a,(CK64SourceLabInput){.reset=true}));assert(a.jumps==0&&a.landings==0&&a.world.total_steps==0&&a.world.grounded);}
static void invalid_atomic(void){CK64SourceLab a=fresh(),before=a;assert(!ck64_source_lab_step(NULL,(CK64SourceLabInput){0}));assert(!ck64_source_lab_step(&a,(CK64SourceLabInput){.stick_y=NAN}));assert(!ck64_source_lab_step(&a,(CK64SourceLabInput){.camera_turn=INFINITY}));assert(!ck64_source_lab_step(&a,(CK64SourceLabInput){.stick_x=10}));assert(memcmp(&a,&before,sizeof a)==0);a.world.total_steps=UINT_MAX;assert(!ck64_source_lab_step(&a,(CK64SourceLabInput){0}));}
int main(void){initialized();idle();actual_horizontal();friction_stop();jump_and_land();early_release();camera_relative();pillar_collision();reset();invalid_atomic();puts("10 source-driven room lab scenario groups passed");return 0;}
