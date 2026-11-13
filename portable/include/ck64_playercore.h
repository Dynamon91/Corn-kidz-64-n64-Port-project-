#ifndef CK64_PLAYERCORE_H
#define CK64_PLAYERCORE_H
/* Portable source-grounded subset of PlayerCtrl from original v1.029 IL.
 * Does not replace PlayerCtrl::Run or the current N64 diagnostic controls. */
#include <stdbool.h>
#include <stdint.h>
typedef struct CK64PlayerVec3 { float x,y,z; } CK64PlayerVec3;
typedef struct CK64PlayerCore {
    int32_t pl_state;
    bool no_control;
    int32_t camera_type;
    float z_mov;
    CK64PlayerVec3 pl_velocity;
    float y_velocity;
    bool on_floor, no_look, no_attack;
    float camera_shake_ud, fall_respawn_time;
} CK64PlayerCore;
bool ck64_playercore_init(CK64PlayerCore *p);
/* PlayerCtrl::Gravity token 0x060002a1: caller supplies decrement and cap. */
bool ck64_playercore_gravity(CK64PlayerCore *p,float decrement,float terminal_speed);
/* PlayerCtrl::ResetPl 0x060002c8 resets five named fields, NOT y_velocity. */
bool ck64_playercore_reset(CK64PlayerCore *p);
/* PlayerCtrl::ZXPos 0x060002ac: retain X,Z and clear Y. */
CK64PlayerVec3 ck64_playercore_zxpos(CK64PlayerVec3 in);
/* Simple setters translated from original method bodies. */
bool ck64_playercore_look_disable(CK64PlayerCore *p);
bool ck64_playercore_look_enable(CK64PlayerCore *p);
bool ck64_playercore_attack_disable(CK64PlayerCore *p);
bool ck64_playercore_camera_shake(CK64PlayerCore *p,float amount);
bool ck64_playercore_set_fall_respawn(CK64PlayerCore *p,float amount);
#endif
