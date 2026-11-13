#include "ck64_source_lab.h"
#include <limits.h>
#include <math.h>

#define CK64_LAB_PI 3.14159265358979323846f
#define CK64_LAB_STEP (1.0f / 30.0f)

static float clampf(float v, float low, float high) {
    return v < low ? low : v > high ? high : v;
}
static bool contact(const CK64World3D *w, float x, float z) {
    const float r=CK64_TEST_PILLAR_HALF+CK64_TEST_AVATAR_HALF;
    return w->position.y < CK64_TEST_PILLAR_HEIGHT &&
           w->position.y+CK64_TEST_AVATAR_HEIGHT>0.0f &&
           x>CK64_TEST_PILLAR_X-r && x<CK64_TEST_PILLAR_X+r &&
           z>CK64_TEST_PILLAR_Z-r && z<CK64_TEST_PILLAR_Z+r;
}
bool ck64_source_lab_init(CK64SourceLab *lab) {
    if(!lab) return false;
    CK64SourceLab next={0};
    ck64_world3d_reset(&next.world);
    if(!ck64_player_motion_init(&next.motion))return false;
    next.state0.on_floor=true;
    next.state0.forward=(CK64PlayerVec3){0,0,1};
    next.state0.last_ctrl_dir=(CK64PlayerVec3){0,0,1};
    next.floor_history.on_floor=true;
    next.floor_history.on_floor_timer=1;
    next.initialized=true;
    *lab=next;
    return true;
}
bool ck64_source_lab_step(CK64SourceLab *lab,CK64SourceLabInput in) {
    if(!lab || !lab->initialized || !isfinite(in.stick_x) ||
       !isfinite(in.stick_y) || !isfinite(in.camera_turn) ||
       fabsf(in.stick_x)>1.25f || fabsf(in.stick_y)>1.25f ||
       fabsf(in.camera_turn)>1.25f || lab->world.total_steps==UINT_MAX)
        return false;
    if(in.reset)return ck64_source_lab_init(lab);
    CK64SourceLab next=*lab;
    CK64World3D *w=&next.world;
    if(w->contact_flash_steps) --w->contact_flash_steps;
    if(w->ejection_flash_steps) --w->ejection_flash_steps;
    w->collided_this_step=false;
    const float sx=clampf(in.stick_x,-1.0f,1.0f);
    const float sy=clampf(in.stick_y,-1.0f,1.0f);
    float mag=sqrtf(sx*sx+sy*sy);
    float ax=sx,az=sy;
    if(mag<0.12f){ax=0.0f;az=0.0f;mag=0.0f;}
    else if(mag>1.0f){ax/=mag;az/=mag;mag=1.0f;}
    w->camera_yaw += clampf(in.camera_turn,-1.0f,1.0f)*1.8f*CK64_LAB_STEP;
    if(w->camera_yaw>CK64_LAB_PI) w->camera_yaw-=2.0f*CK64_LAB_PI;
    if(w->camera_yaw< -CK64_LAB_PI)w->camera_yaw+=2.0f*CK64_LAB_PI;
    const float sn=sinf(w->camera_yaw),cs=cosf(w->camera_yaw);
    const CK64PlayerVec3 direction={cs*ax+sn*az,0.0f,-sn*ax+cs*az};
    next.motion.ctrl_dir=direction;
    next.motion.floor_norm=(CK64PlayerVec3){0,1,0};
    if(mag>0.0f) {
        next.state0.last_ctrl_dir=direction;
        const float heading=atan2f(direction.x,direction.z)*0.5f;
        next.motion.ctrl_rot=(CK64Quat){0,sinf(heading),0,cosf(heading)};
    }
    /* These are source-derived Run fields; input/camera are adapter-only. */
    next.state0.on_floor=w->grounded;
    next.state0.on_floor_timer=next.floor_history.on_floor_timer;
    next.state0.z_mov=next.motion.z_mov;
    next.state0.add_vel_magnitude=0.0f; /* air push not yet extracted */
    next.state0.forward=(CK64PlayerVec3){
        2.0f*(next.motion.rotation.x*next.motion.rotation.z+
              next.motion.rotation.w*next.motion.rotation.y),0.0f,
        1.0f-2.0f*(next.motion.rotation.x*next.motion.rotation.x+
                  next.motion.rotation.y*next.motion.rotation.y)};
    CK64State0Result decision={0};
    if(!ck64_player_state0_step(&next.state0,
                               (CK64State0Buttons){
                                   .a_down=in.jump_press,
                                   .a_up=in.jump_release},&decision))
        return false;
    if(!ck64_player_motion_step(&next.motion,decision.move))return false;
    if((decision.effects&CK64_S0_ANIM_JUMP)!=0) {
        if(next.jumps==UINT_MAX)return false;
        ++next.jumps;
    }
    float nx=clampf(w->position.x+next.motion.pl_vel.x,-7.5f,7.5f);
    if(nx==7.5f||nx== -7.5f)next.motion.pl_vel.x=0.0f;
    if(w->collision_enabled&&contact(w,nx,w->position.z)) {
        const float r=CK64_TEST_PILLAR_HALF+CK64_TEST_AVATAR_HALF;
        nx=CK64_TEST_PILLAR_X+(next.motion.pl_vel.x>0.0f?-r:r);
        next.motion.pl_vel.x=0.0f;w->collided_this_step=true;
    }
    w->position.x=nx;
    float nz=clampf(w->position.z+next.motion.pl_vel.z,-7.5f,7.5f);
    if(nz==7.5f||nz== -7.5f)next.motion.pl_vel.z=0.0f;
    if(w->collision_enabled&&contact(w,w->position.x,nz)) {
        const float r=CK64_TEST_PILLAR_HALF+CK64_TEST_AVATAR_HALF;
        nz=CK64_TEST_PILLAR_Z+(next.motion.pl_vel.z>0.0f?-r:r);
        next.motion.pl_vel.z=0.0f;w->collided_this_step=true;
    }
    w->position.z=nz;
    if(w->collided_this_step) {
        if(w->collision_hits<UINT_MAX)++w->collision_hits;
        if(!w->contact_last_step) {
            if(w->contact_events<UINT_MAX)++w->contact_events;
            w->contact_flash_steps=10;
        }
    }
    w->contact_last_step=w->collided_this_step;
    /* Original Run uses yVel in transform translation; this simplified flat
     * ground is NOT the original collider, timestep or Unity character motor. */
    w->position.y+=next.state0.y_vel;
    if(w->position.y<=0.0f&&next.state0.y_vel<=0.0f) {
        w->position.y=0.0f;
        next.state0.y_vel=0.0f;
        w->grounded=true;
    } else w->grounded=false;
    w->velocity=(CK64Vec3){next.motion.pl_vel.x*30.0f,
                            next.state0.y_vel*30.0f,
                            next.motion.pl_vel.z*30.0f};
    next.floor_history.on_floor=w->grounded;
    CK64PlayerRunFooterResult history={0};
    if(!ck64_player_run_footer_step(&next.floor_history,&history))
        return false;
    if(history.landing_edge) {
        if(next.landings==UINT_MAX)return false;
        ++next.landings;
    }
    if(!isfinite(w->position.x)||!isfinite(w->position.y)||
       !isfinite(w->position.z)||!isfinite(w->camera_yaw)||
       !isfinite(next.motion.z_mov)||!isfinite(next.state0.y_vel))
        return false;
    next.last_decision=decision;
    ++w->total_steps;
    *lab=next;
    return true;
}
