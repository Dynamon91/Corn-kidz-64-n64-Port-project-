#ifndef CK64_PLAYER_STATE0_H
#define CK64_PLAYER_STATE0_H
/* Source-grounded subset of original PlayerCtrl::Run (state 0).
 * NOT actual Unity physics/levels. */
#include <stdbool.h>
#include <stdint.h>
#include "ck64_player_motion.h"
typedef enum CK64State0MotionPath {
    CK64_S0_GROUND_SLOW, CK64_S0_GROUND_FAST,
    CK64_S0_AIR_NO_PUSH, CK64_S0_AIR_PUSH
} CK64State0MotionPath;
enum CK64State0Effect {
    CK64_S0_INTERACT=1u<<0, CK64_S0_ANIM_JUMP=1u<<1,
    CK64_S0_SFX_JUMP=1u<<2, CK64_S0_ANIM_SPIN=1u<<3,
    CK64_S0_SFX_SPIN=1u<<4, CK64_S0_ANIM_JUMPF=1u<<5,
    CK64_S0_STEP8=1u<<6, CK64_S0_ANIM_STAND2=1u<<7
};
typedef struct CK64State0 {
    int32_t pl_state, jump_num, on_floor_timer, slip_timer;
    bool on_floor;
    float y_vel, z_mov, add_vel_magnitude;
    CK64PlayerVec3 last_ctrl_dir, forward;
} CK64State0;
typedef struct CK64State0Buttons {
    bool a_down, a_up, x_down, near_obj_present, near_obj_interactable;
    bool fliploop_playing, flip_playing;
} CK64State0Buttons;
typedef struct CK64State0Result {
    CK64State0MotionPath path;
    CK64MoveParams move; /* MovPl call arguments; not live physics */
    uint32_t effects;
    bool early_return;
} CK64State0Result;
bool ck64_player_state0_step(CK64State0 *state,
                            CK64State0Buttons buttons,
                            CK64State0Result *out);
#endif
