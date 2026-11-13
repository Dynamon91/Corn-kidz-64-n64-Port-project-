#include "ck64_player_state0.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int eq(float a,float b){return fabsf(a-b)<.00002f;}
static CK64State0 fresh(void){
 CK64State0 s={0};s.on_floor=true;s.z_mov=.4f;
 s.forward=(CK64PlayerVec3){0,0,1};
 s.last_ctrl_dir=(CK64PlayerVec3){1,0,0};return s;
}
static void ground(void){
 CK64State0 s=fresh();CK64State0Result r;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){0},&r));
 assert(r.path==CK64_S0_GROUND_SLOW&&eq(s.y_vel,-.1f));
 assert(eq(r.move.turn_step,20)&&eq(r.move.acceleration,.02f));
 assert(eq(r.move.brake,.01f)&&eq(r.move.speed_scale,.16f));
 assert(!r.move.direct&&!r.move.fractional_turn);
}
static void fast(void){
 CK64State0 s=fresh();CK64State0Result r;s.z_mov=.6f;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){0},&r));
 assert(r.path==CK64_S0_GROUND_FAST&&r.move.direct);
 assert(r.move.fractional_turn&&eq(r.move.turn_step,.14f));
}
static void air(void){
 CK64State0 s=fresh();CK64State0Result r;
 s.on_floor=false;s.y_vel=.2f;s.add_vel_magnitude=.099f;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){0},&r));
 assert(r.path==CK64_S0_AIR_NO_PUSH&&eq(r.move.acceleration,.01f));
 assert(eq(s.y_vel,.18f));
 s.add_vel_magnitude=.1f;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){0},&r));
 assert(r.path==CK64_S0_AIR_PUSH&&eq(r.move.acceleration,0));
}
static void jump(void){
 CK64State0 s=fresh();CK64State0Result r;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_down=true},&r));
 assert(eq(s.y_vel,.23f)&&!s.on_floor&&s.jump_num==1);
 assert(r.effects&CK64_S0_ANIM_JUMP);
 assert(r.effects&CK64_S0_SFX_JUMP);
 assert(r.effects&CK64_S0_STEP8);
}
static void coyote(void){
 CK64State0 s=fresh();CK64State0Result r;
 s.on_floor=false;s.on_floor_timer=-3;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_down=true},&r));
 assert(eq(s.y_vel,.23f));
 s=fresh();s.on_floor=false;s.on_floor_timer=-4;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_down=true},&r));
 assert(eq(s.y_vel,-.02f)&&r.effects==0);
 s=fresh();s.slip_timer=1;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_down=true},&r));
 assert(eq(s.y_vel,-.1f)&&r.effects==0);
}
static void spin(void){
 CK64State0 s=fresh();CK64State0Result r;s.z_mov=.8f;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_down=true},&r));
 assert(eq(s.y_vel,.23f)&&(r.effects&CK64_S0_ANIM_JUMPF));
 s=fresh();s.z_mov=.8f;s.jump_num=1;s.on_floor_timer=5;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_down=true},&r));
 assert(eq(s.y_vel,.24f)&&s.jump_num==0);
 assert(r.effects&CK64_S0_ANIM_SPIN);
 s=fresh();s.jump_num=1;s.on_floor_timer=5;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_down=true},&r));
 assert(eq(s.y_vel,.23f)&&s.jump_num==0);
 assert(!(r.effects&CK64_S0_ANIM_SPIN));
}
static void short_hop(void){
 CK64State0 s=fresh();CK64State0Result r;s.on_floor=false;s.y_vel=.2f;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_up=true},&r));
 assert(eq(s.y_vel,.1f));
 s=fresh();s.on_floor=false;s.y_vel=.2f;
 assert(ck64_player_state0_step(&s,
  (CK64State0Buttons){.a_up=true,.flip_playing=true},&r));
 assert(eq(s.y_vel,.18f));
}
static void interact(void){
 CK64State0 s=fresh();CK64State0Result r;
 CK64State0Buttons b={.a_down=true,.x_down=true,
   .near_obj_present=true,.near_obj_interactable=true};
 assert(ck64_player_state0_step(&s,b,&r));
 assert(r.early_return&&s.pl_state==5&&eq(s.y_vel,0));
 assert(s.jump_num==0&&(r.effects&CK64_S0_INTERACT));
 s=fresh();b.near_obj_interactable=false;
 assert(ck64_player_state0_step(&s,b,&r));
 assert(s.pl_state==0&&eq(s.y_vel,.23f));
}
static void invalid(void){
 CK64State0 s=fresh(),before=s;CK64State0Result r={0},old=r;
 assert(!ck64_player_state0_step(NULL,(CK64State0Buttons){0},&r));
 assert(!ck64_player_state0_step(&s,(CK64State0Buttons){0},NULL));
 s.z_mov=NAN;
 assert(!ck64_player_state0_step(&s,(CK64State0Buttons){0},&r));
 s=before;s.pl_state=8;
 assert(!ck64_player_state0_step(&s,(CK64State0Buttons){0},&r));
 s=before;s.jump_num=INT32_MAX;
 assert(!ck64_player_state0_step(&s,(CK64State0Buttons){.a_down=true},&r));
 s=before;assert(memcmp(&s,&before,sizeof s)==0);
 assert(memcmp(&r,&old,sizeof r)==0);
}
int main(void){
 ground();fast();air();jump();coyote();spin();short_hop();interact();invalid();
 puts("9 original state-0 test groups pass");return 0;
}
