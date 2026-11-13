#ifndef CK64_PLAYER_MOTION_H
#define CK64_PLAYER_MOTION_H
/* PlayerCtrl::MovPl (v1.029, token 0x060002a0).
 * Source-grounded order, but numerical parity with Unity unverified.
 * No original assets or Run state machine are included. */
#include "ck64_playercore.h"
#include <stdbool.h>
typedef struct CK64Quat {float x,y,z,w;} CK64Quat;
typedef struct CK64PlayerMotion {
    CK64PlayerVec3 ctrl_dir, pl_vel, floor_norm;
    CK64Quat rotation, ctrl_rot;
    float z_mov;
} CK64PlayerMotion;
typedef struct CK64MoveParams {
    CK64PlayerVec3 movement; /* IL arg1: caller-supplied direction */
    float turn_step;         /* IL arg2: degrees or fraction of remaining angle */
    float acceleration;      /* IL arg3 */
    float brake;             /* IL arg4 */
    bool direct;             /* IL arg5 */
    bool fractional_turn;    /* IL arg6 */
    float speed_scale;       /* IL arg7 */
} CK64MoveParams;
bool ck64_player_motion_init(CK64PlayerMotion *motion);
/* C11 PAL; no deltaTime scaling; invalid arguments leave state unchanged. */
bool ck64_player_motion_step(CK64PlayerMotion *motion, CK64MoveParams params);
#endif
