#include "ck64_player_motion.h"
#include <math.h>
#include <stdint.h>
#define CK64_PI 3.14159265358979323846f
#define CK64_RAD_TO_DEG (180.0f / CK64_PI)
static CK64PlayerVec3 add(CK64PlayerVec3 a,CK64PlayerVec3 b) {
    return (CK64PlayerVec3){a.x+b.x,a.y+b.y,a.z+b.z};
}
static CK64PlayerVec3 mul(CK64PlayerVec3 a,float v) {
    return (CK64PlayerVec3){a.x*v,a.y*v,a.z*v};
}
static float dot(CK64PlayerVec3 a,CK64PlayerVec3 b) {
    return a.x*b.x+a.y*b.y+a.z*b.z;
}
static CK64PlayerVec3 cross(CK64PlayerVec3 a,CK64PlayerVec3 b) {
    return (CK64PlayerVec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
static bool finite_vec(CK64PlayerVec3 a) {
    return isfinite(a.x)&&isfinite(a.y)&&isfinite(a.z);
}
static bool finite_quat(CK64Quat q) {
    return isfinite(q.x)&&isfinite(q.y)&&isfinite(q.z)&&isfinite(q.w);
}
static float mag(CK64PlayerVec3 a) {return sqrtf(dot(a,a));}
static CK64PlayerVec3 normalized(CK64PlayerVec3 a) {
    const float m=mag(a);
    return m>0.00001f?mul(a,1.0f/m):(CK64PlayerVec3){0,0,0};
}
static float clamp(float a,float lo,float hi) {return a<lo?lo:a>hi?hi:a;}
static float towards(float a,float b,float delta) {
    if(fabsf(b-a)<=delta)return b;
    return a+copysignf(delta,b-a);
}
static CK64PlayerVec3 vec_towards_zero(CK64PlayerVec3 v,float delta) {
    const float m=mag(v);
    if(m<=delta||m==0.0f)return (CK64PlayerVec3){0,0,0};
    return mul(v,(m-delta)/m);
}
static CK64PlayerVec3 lerp_vec(CK64PlayerVec3 a,CK64PlayerVec3 b,float t) {
    return add(a,mul(add(b,mul(a,-1.0f)),clamp(t,0.0f,1.0f)));
}
static CK64Quat quat_normalized(CK64Quat a) {
    const float n=sqrtf(a.x*a.x+a.y*a.y+a.z*a.z+a.w*a.w);
    if(n<=1.0e-7f)return (CK64Quat){0,0,0,1};
    return (CK64Quat){a.x/n,a.y/n,a.z/n,a.w/n};
}
static CK64Quat quat_mul(CK64Quat a,CK64Quat b) {
    return (CK64Quat){
        a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
        a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
        a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
        a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z
    };
}
static CK64PlayerVec3 quat_rotate(CK64Quat q,CK64PlayerVec3 v) {
    q=quat_normalized(q);
    const CK64PlayerVec3 u={q.x,q.y,q.z};
    return add(v,add(mul(cross(u,v),2.0f*q.w),
                      mul(cross(u,cross(u,v)),2.0f)));
}
static CK64Quat from_to(CK64PlayerVec3 a,CK64PlayerVec3 b) {
    a=normalized(a);b=normalized(b);
    const float d=clamp(dot(a,b),-1.0f,1.0f);
    if(d>0.999999f)return (CK64Quat){0,0,0,1};
    if(d< -0.999999f) {
        CK64PlayerVec3 axis=cross((CK64PlayerVec3){1,0,0},a);
        if(mag(axis)<0.0001f)axis=cross((CK64PlayerVec3){0,1,0},a);
        axis=normalized(axis);
        return (CK64Quat){axis.x,axis.y,axis.z,0};
    }
    const CK64PlayerVec3 c=cross(a,b);
    return quat_normalized((CK64Quat){c.x,c.y,c.z,1.0f+d});
}
static float quat_dot(CK64Quat a,CK64Quat b) {
    return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
}
static CK64Quat quat_slerp(CK64Quat a,CK64Quat b,float t) {
    a=quat_normalized(a);b=quat_normalized(b);
    float d=quat_dot(a,b);
    if(d<0.0f){b=(CK64Quat){-b.x,-b.y,-b.z,-b.w};d=-d;}
    t=clamp(t,0.0f,1.0f);
    float s0=1.0f-t,s1=t;
    if(d<0.9995f){
        const float omega=acosf(clamp(d,-1.0f,1.0f));
        const float denominator=sinf(omega);
        if(fabsf(denominator)>1.0e-6f) {
            s0=sinf((1.0f-t)*omega)/denominator;
            s1=sinf(t*omega)/denominator;
        }
    }
    return quat_normalized((CK64Quat){s0*a.x+s1*b.x,s0*a.y+s1*b.y,
                                    s0*a.z+s1*b.z,s0*a.w+s1*b.w});
}
static CK64Quat rotate_towards(CK64Quat a,CK64Quat b,float max_degrees) {
    a=quat_normalized(a);b=quat_normalized(b);
    const float d=clamp(fabsf(quat_dot(a,b)),0.0f,1.0f);
    const float angle=2.0f*acosf(d)*CK64_RAD_TO_DEG;
    if(angle<0.00001f)return b;
    return quat_slerp(a,b,clamp(max_degrees/angle,0.0f,1.0f));
}
bool ck64_player_motion_init(CK64PlayerMotion *m) {
    if(!m)return false;
    *m=(CK64PlayerMotion){0};
    m->floor_norm=(CK64PlayerVec3){0,1,0};
    m->rotation=(CK64Quat){0,0,0,1};
    m->ctrl_rot=(CK64Quat){0,0,0,1};
    return true;
}
bool ck64_player_motion_step(CK64PlayerMotion *m,CK64MoveParams p) {
    if(!m||!finite_vec(m->ctrl_dir)||!finite_vec(m->pl_vel)||
       !finite_vec(m->floor_norm)||!finite_vec(p.movement)||
       !finite_quat(m->rotation)||!finite_quat(m->ctrl_rot)||
       !isfinite(m->z_mov)||!isfinite(p.turn_step)||
       !isfinite(p.acceleration)||!isfinite(p.brake)||!isfinite(p.speed_scale)||
       p.acceleration<0.0f||p.brake<0.0f||p.turn_step<0.0f||
       p.speed_scale<=0.0f||mag(m->floor_norm)<0.00001f||
       mag(p.movement)>1000000.0f||mag(m->ctrl_dir)>1000000.0f||
       mag(m->pl_vel)>1000000.0f)return false;
    CK64PlayerMotion next=*m;
    const CK64PlayerVec3 up={0,1,0};
    CK64PlayerVec3 current_up=quat_rotate(next.rotation,up);
    const CK64Quat tilt=from_to(current_up,next.floor_norm);
    const CK64PlayerVec3 direction=quat_rotate(tilt,p.movement);
    if(p.direct) {
        if(mag(next.ctrl_dir)>=0.6f)next.ctrl_dir=normalized(next.ctrl_dir);
        const float target=mag(next.ctrl_dir);
        if(next.z_mov<target)
            next.z_mov=towards(next.z_mov,target,p.acceleration);
        else next.z_mov+=(target-next.z_mov)*clamp(p.brake,0.0f,1.0f);
        next.pl_vel=mul(direction,next.z_mov*p.speed_scale);
    } else {
        next.pl_vel=mul(normalized(next.pl_vel),next.z_mov*p.speed_scale);
        next.pl_vel.y=0.0f;
        if(mag(next.pl_vel)<mag(next.ctrl_dir)*p.speed_scale)
            next.pl_vel=add(next.pl_vel,mul(direction,p.acceleration));
        next.pl_vel=vec_towards_zero(next.pl_vel,p.brake);
        if(mag(next.pl_vel)>p.speed_scale)
            next.pl_vel=lerp_vec(next.pl_vel,
                      mul(normalized(next.pl_vel),p.speed_scale),0.1f);
        next.z_mov=mag(next.pl_vel)/p.speed_scale;
    }
    if(next.ctrl_dir.x!=0.0f||next.ctrl_dir.y!=0.0f||next.ctrl_dir.z!=0.0f){
        float step=p.turn_step;
        if(p.fractional_turn){
            const CK64Quat a=quat_normalized(next.rotation);
            const CK64Quat b=quat_normalized(next.ctrl_rot);
            const float d=clamp(fabsf(quat_dot(a,b)),0.0f,1.0f);
            step=2.0f*acosf(d)*CK64_RAD_TO_DEG*p.turn_step;
        }
        next.rotation=rotate_towards(next.rotation,next.ctrl_rot,step);
    }
    current_up=quat_rotate(next.rotation,up);
    next.rotation=quat_normalized(quat_mul(from_to(current_up,up),next.rotation));
    if(!finite_vec(next.pl_vel)||!isfinite(next.z_mov)||
       !finite_quat(next.rotation))return false;
    *m=next;
    return true;
}
