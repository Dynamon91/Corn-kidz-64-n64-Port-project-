#include "ck64_player_state0.h"
#include "ck64_playercore.h"
#include <limits.h>
#include <math.h>
static bool finite_vec(CK64PlayerVec3 v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}
static CK64MoveParams mov(CK64PlayerVec3 v,float turn,float accel,
                          float brake,bool direct,bool fractional) {
    CK64MoveParams p={v,turn,accel,brake,direct,fractional,0.16f};
    return p;
}
bool ck64_player_state0_step(CK64State0 *s,CK64State0Buttons b,
                            CK64State0Result *out) {
    if(!s||!out||s->pl_state!=0||!isfinite(s->y_vel)||
       !isfinite(s->z_mov)||!isfinite(s->add_vel_magnitude)||
       s->add_vel_magnitude<0.0f||!finite_vec(s->last_ctrl_dir)||
       !finite_vec(s->forward)||
       (b.a_down&&(s->on_floor||s->on_floor_timer> -4)&&
        s->slip_timer==0&&s->jump_num==INT32_MAX))return false;
    CK64State0 n=*s;CK64State0Result r={0};
    CK64PlayerCore g={0};g.on_floor=n.on_floor;g.y_velocity=n.y_vel;
    if(!ck64_playercore_gravity(&g,0.02f,0.6f))return false;
    n.y_vel=g.y_velocity;
    if(n.on_floor&&n.z_mov<0.6f){
        r.path=CK64_S0_GROUND_SLOW;
        r.move=mov(n.last_ctrl_dir,20.0f,0.02f,0.01f,false,false);
    }else if(n.on_floor){
        r.path=CK64_S0_GROUND_FAST;
        r.move=mov(n.forward,0.2f-n.z_mov*0.1f,0.05f,0.1f,true,true);
    }else if(n.add_vel_magnitude<0.1f){
        r.path=CK64_S0_AIR_NO_PUSH;
        r.move=mov(n.last_ctrl_dir,0.07f,0.01f,0.005f,false,true);
    }else{
        r.path=CK64_S0_AIR_PUSH;
        r.move=mov(n.last_ctrl_dir,0.07f,0.0f,0.005f,false,true);
    }
    if(b.x_down&&n.on_floor&&b.near_obj_present&&b.near_obj_interactable){
        n.pl_state=5;n.y_vel=0.0f;
        r.effects=CK64_S0_INTERACT|CK64_S0_ANIM_STAND2;
        r.early_return=true;
    }else{
        if(b.a_down&&(n.on_floor||n.on_floor_timer> -4)&&n.slip_timer==0){
            n.y_vel=0.23f;++n.jump_num;
            r.effects|=CK64_S0_ANIM_JUMP|CK64_S0_SFX_JUMP;
            if(n.on_floor_timer>0&&n.on_floor_timer<11&&n.jump_num>=2){
                n.jump_num=0;
                if(n.z_mov>0.6f){
                    n.y_vel=0.24f;
                    r.effects|=CK64_S0_ANIM_SPIN|CK64_S0_SFX_SPIN;
                }
            }else if(n.z_mov>0.6f)r.effects|=CK64_S0_ANIM_JUMPF;
            n.on_floor=false;r.effects|=CK64_S0_STEP8;
        }
        if(b.a_up&&n.y_vel>0.15f&&!b.fliploop_playing&&
           !b.flip_playing)n.y_vel=0.1f;
    }
    if(!isfinite(n.y_vel)||!isfinite(r.move.turn_step))return false;
    *s=n;*out=r;return true;
}
