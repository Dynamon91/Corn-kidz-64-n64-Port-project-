#include "ck64_player_footer.h"
#include "ck64_player_state0.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void ground(void) {
 CK64PlayerRunFooter s={.on_floor=true};CK64PlayerRunFooterResult r;
 assert(ck64_player_run_footer_step(&s,&r));
 assert(s.on_floor_timer==1&&s.pl_timer==1&&s.pl_timer2==1&&s.pl_timer3==1);
 assert(!r.landing_edge&&!r.left_floor);
 assert(ck64_player_run_footer_step(&s,&r)&&s.on_floor_timer==2);
}
static void air(void) {
 CK64PlayerRunFooter s={0};CK64PlayerRunFooterResult r;
 assert(ck64_player_run_footer_step(&s,&r)&&s.on_floor_timer==-1&&!r.left_floor);
 assert(ck64_player_run_footer_step(&s,&r)&&s.on_floor_timer==-2);
}
static void leave(void) {
 CK64PlayerRunFooter s={.on_floor_timer=12};CK64PlayerRunFooterResult r;
 assert(ck64_player_run_footer_step(&s,&r));
 assert(s.on_floor_timer==-1&&r.left_floor&&!r.landing_edge);
 assert(ck64_player_run_footer_step(&s,&r)&&s.on_floor_timer==-2&&!r.left_floor);
}
static void land(void) {
 CK64PlayerRunFooter s={.on_floor_timer=-10,.on_floor=true};CK64PlayerRunFooterResult r;
 assert(ck64_player_run_footer_step(&s,&r));
 assert(s.on_floor_timer==1&&r.landing_edge&&!r.left_floor);
 assert(ck64_player_run_footer_step(&s,&r)&&s.on_floor_timer==2&&!r.landing_edge);
}
static void coyote(void) {
 CK64PlayerRunFooter s={.on_floor_timer=10};CK64PlayerRunFooterResult r;
 for(int i=-1;i>=-4;--i){
  assert(ck64_player_run_footer_step(&s,&r)&&s.on_floor_timer==i);
  assert((s.on_floor_timer > -4)==(i>=-3));
 }
}
static void wall_ground(void) {
 CK64PlayerRunFooter s={.on_floor=true,.last_wall_object_present=true};
 CK64PlayerRunFooterResult r;
 assert(ck64_player_run_footer_step(&s,&r)&&!s.last_wall_object_present&&r.cleared_wall);
 assert(ck64_player_run_footer_step(&s,&r)&&!r.cleared_wall);
}
static void wall_states(void) {
 CK64PlayerRunFooterResult r;
 for(int i=0;i<=14;++i){
  CK64PlayerRunFooter s={.pl_state=i,.last_wall_object_present=true};
  assert(ck64_player_run_footer_step(&s,&r));
  bool special=i==4||i==11||i==12;
  assert(s.last_wall_object_present!=special&&r.cleared_wall==special);
 }
}
static void counters(void) {
 CK64PlayerRunFooter s={.pl_timer=-90,.pl_timer2=5,.pl_timer3=-20,
                        .on_floor=true,.no_air=true};
 CK64PlayerRunFooterResult r;
 assert(ck64_player_run_footer_step(&s,&r));
 assert(s.pl_timer==-89&&s.pl_timer2==6&&s.pl_timer3==-19&&!s.no_air);
}
static void overflow(void) {
 CK64PlayerRunFooter s={.pl_timer=INT32_MAX},old=s;
 CK64PlayerRunFooterResult r={true,true,true},before=r;
 assert(!ck64_player_run_footer_step(&s,&r));
 assert(memcmp(&s,&old,sizeof s)==0&&memcmp(&r,&before,sizeof r)==0);
 s=(CK64PlayerRunFooter){.pl_timer2=INT32_MAX};assert(!ck64_player_run_footer_step(&s,&r));
 s=(CK64PlayerRunFooter){.pl_timer3=INT32_MAX};assert(!ck64_player_run_footer_step(&s,&r));
 s=(CK64PlayerRunFooter){.on_floor=true,.on_floor_timer=INT32_MAX};
 assert(!ck64_player_run_footer_step(&s,&r));
 s=(CK64PlayerRunFooter){.on_floor=false,.on_floor_timer=INT32_MIN};
 assert(!ck64_player_run_footer_step(&s,&r));
 assert(!ck64_player_run_footer_step(NULL,&r));
 assert(!ck64_player_run_footer_step(&s,NULL));
 assert(memcmp(&r,&before,sizeof r)==0);
}
static void no_false_landing(void) {
 CK64PlayerRunFooter s={.on_floor=true,.on_floor_timer=1};CK64PlayerRunFooterResult r;
 assert(ck64_player_run_footer_step(&s,&r)&&!r.landing_edge);
 s.on_floor=false;assert(ck64_player_run_footer_step(&s,&r)&&r.left_floor);
 s.on_floor=true;assert(ck64_player_run_footer_step(&s,&r)&&r.landing_edge);
}
static void source_jump_link(void) {
 CK64PlayerRunFooter f={.on_floor=true,.on_floor_timer=2};
 CK64State0 s={.on_floor=true,.on_floor_timer=2,.z_mov=.4f,
                .forward={0,0,1},.last_ctrl_dir={0,0,1}};
 CK64State0Result jr={0};CK64PlayerRunFooterResult fr={0};
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_down=true},&jr));
 assert(fabsf(s.y_vel-.23f)<.00001f&&!s.on_floor&&s.jump_num==1);
 f.on_floor=s.on_floor;
 assert(ck64_player_run_footer_step(&f,&fr)&&f.on_floor_timer==-1&&fr.left_floor);
 s.on_floor_timer=f.on_floor_timer;
 assert(ck64_player_state0_step(&s,(CK64State0Buttons){.a_up=true},&jr));
 assert(fabsf(s.y_vel-.1f)<.00001f);
 assert(ck64_player_run_footer_step(&f,&fr)&&f.on_floor_timer==-2);
 f.on_floor=true;
 assert(ck64_player_run_footer_step(&f,&fr)&&f.on_floor_timer==1&&fr.landing_edge);
}
int main(void) {
 ground();air();leave();land();coyote();wall_ground();wall_states();
 counters();overflow();no_false_landing();source_jump_link();
 puts("11 PlayerCtrl.Run footer scenario groups passed");return 0;
}
