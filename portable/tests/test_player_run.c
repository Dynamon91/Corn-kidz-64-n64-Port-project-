#include "ck64_player_run.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static CK64RunPrelude initial(void) {
    CK64RunPrelude s;
    assert(ck64_player_run_init(&s));
    return s;
}
static void initialization(void) {
    CK64RunPrelude s=initial();
    assert(s.camera_type==1 && s.pl_state==0);
    assert(!ck64_player_run_init(NULL));
}
static void state_entry_changes_only(void) {
    CK64RunPrelude s=initial();uint32_t e=17;
    s.pl_state=8;s.change_state=3;s.last_state=2;
    s.pl_timer=15;s.pl_timer2=14;s.pl_timer3=13;s.pl_counter=12;
    s.pl_float=0.5f;s.pl_switch=7;s.no_hurt=s.no_collision=s.no_look=s.wall_attached=true;
    assert(ck64_player_run_prelude(&s,&e));
    assert(e==CK64_RUN_ENTRY_BASE_EFFECTS);
    assert(s.last_state==3 && s.change_state==8);
    assert(s.pl_timer==0 && s.pl_timer2==0 && s.pl_timer3==0 && s.pl_counter==0);
    assert(s.pl_float==0 && s.pl_switch==7);
    assert(!s.no_hurt && !s.no_collision && !s.no_look && !s.wall_attached);
    s.pl_timer=5;s.pl_counter=3;s.pl_float=1;
    assert(ck64_player_run_prelude(&s,&e));
    assert(e==0 && s.pl_timer==5 && s.pl_counter==3 && s.pl_float==1);
}
static void previous_state_sfx(void) {
    CK64RunPrelude s=initial();uint32_t e=0;
    s.change_state=46;s.pl_state=7;
    assert(ck64_player_run_prelude(&s,&e));
    assert((e&CK64_RUN_STOP_SFX_53) && !(e&CK64_RUN_STOP_SFX_55));
    s.change_state=45;s.pl_state=9;
    assert(ck64_player_run_prelude(&s,&e));
    assert((e&CK64_RUN_STOP_SFX_55) && !(e&CK64_RUN_STOP_SFX_53));
    s.change_state=9;s.pl_state=46;
    assert(ck64_player_run_prelude(&s,&e));
    assert((e&CK64_RUN_STOP_SFX_55)==0 && (e&CK64_RUN_STOP_SFX_53)==0);
}
static void switch_timer_exactly_one(void) {
    CK64RunPrelude s=initial();uint32_t e;
    s.pl_switch=77;s.pl_timer=0;
    assert(ck64_player_run_prelude(&s,&e) && s.pl_switch==77);
    s.pl_timer=1;
    assert(ck64_player_run_prelude(&s,&e) && s.pl_switch==0);
    s.pl_switch=12;s.pl_timer=2;
    assert(ck64_player_run_prelude(&s,&e) && s.pl_switch==12);
}
static void input_lock_is_selective(void) {
    CK64RunPrelude s=initial();uint32_t e=0;
    s.no_control=true;
    s.input.a_down=s.input.a_up=s.input.a_hold=true;
    s.input.x_down=s.input.x_hold=true;
    s.input.b_down=s.input.b_hold=true;
    s.input.y_down=s.input.y_hold=true;
    s.input.rb_hold=s.input.lb_hold=s.input.lb_down=true;
    s.input.ctrl_x=0.6f;s.input.ctrl_y=-0.2f;
    s.input.a_timer=9;s.input.rt_timer=8;s.input.lt_timer=7;
    assert(ck64_player_run_prelude(&s,&e));
    assert(!s.input.a_down && !s.input.a_up && !s.input.a_hold);
    assert(!s.input.x_down && !s.input.x_hold && !s.input.b_down);
    assert(!s.input.y_down && !s.input.y_hold);
    assert(s.input.a_timer==0 && s.input.rt_timer==0 && s.input.lt_timer==0);
    assert(s.input.ctrl_x==0 && s.input.ctrl_y==0);
    assert(s.input.b_hold && s.input.rb_hold && s.input.lb_hold && s.input.lb_down);
}
static void hold_timer_frame_units(void) {
    CK64RunPrelude s=initial();uint32_t e;
    s.input.a_hold=true;s.input.a_timer=2;
    assert(ck64_player_run_prelude(&s,&e) && s.input.a_timer==3);
    assert(ck64_player_run_prelude(&s,&e) && s.input.a_timer==4);
    s.input.a_hold=false;
    assert(ck64_player_run_prelude(&s,&e) && s.input.a_timer==0);
}
static void underwater_camera_priority(void) {
    CK64RunPrelude s=initial();uint32_t e;
    s.underwater=true;s.camera_type=1;s.pl_state=18;s.change_state=18;
    assert(ck64_player_run_prelude(&s,&e) && s.camera_type==2);
    s.camera_type=4;
    assert(ck64_player_run_prelude(&s,&e) && s.camera_type==3);
    s.underwater=false;s.pl_state=0;s.change_state=0;s.camera_type=4;
    assert(ck64_player_run_prelude(&s,&e) && s.camera_type==4);
}
static void camera_zero_and_default(void) {
    CK64RunPrelude s=initial();uint32_t e;
    s.camera_type=0;
    assert(ck64_player_run_prelude(&s,&e) && s.camera_type==0);
    s.camera_type=9;
    assert(ck64_player_run_prelude(&s,&e) && s.camera_type==1);
    s.pl_state=18;s.change_state=18;
    assert(ck64_player_run_prelude(&s,&e) && s.camera_type==3);
}
static void rumble_cooldown_then_fade(void) {
    CK64RunPrelude s=initial();uint32_t e;
    s.rumble_timer=2;s.rumble_l=1;s.rumble_r=0.6f;s.rumble_fade=0.2f;
    assert(ck64_player_run_prelude(&s,&e) && s.rumble_timer==1 && s.rumble_l==1);
    assert(ck64_player_run_prelude(&s,&e) && s.rumble_timer==0 && s.rumble_l==1);
    assert(ck64_player_run_prelude(&s,&e) && fabsf(s.rumble_l-0.8f)<0.0001f);
    assert(fabsf(s.rumble_r-0.4f)<0.0001f);
    s.rumble_l=-0.1f;s.rumble_r=0.05f;
    assert(ck64_player_run_prelude(&s,&e) && s.rumble_l==0 && s.rumble_r==0);
}
static void invalid_values_atomic(void) {
    CK64RunPrelude s=initial();uint32_t e=1000;
    s.rumble_l=NAN;CK64RunPrelude before=s;
    assert(!ck64_player_run_prelude(&s,&e) && e==1000);
    assert(memcmp(&s,&before,sizeof s)==0);
    s.rumble_l=0;s.rumble_fade=-1;
    assert(!ck64_player_run_prelude(&s,&e) && e==1000);
    s.rumble_fade=0;s.input.a_hold=true;s.input.a_timer=INT32_MAX;
    assert(!ck64_player_run_prelude(&s,&e) && e==1000);
    assert(!ck64_player_run_prelude(NULL,&e));
    assert(!ck64_player_run_prelude(&s,NULL));
}
int main(void) {
    initialization();state_entry_changes_only();previous_state_sfx();
    switch_timer_exactly_one();input_lock_is_selective();
    hold_timer_frame_units();underwater_camera_priority();
    camera_zero_and_default();rumble_cooldown_then_fade();
    invalid_values_atomic();
    puts("10 PlayerCtrl::Run prelude scenario groups passed");
    return 0;
}
