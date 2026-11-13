#ifndef CK64_PLAYER_RUN_H
#define CK64_PLAYER_RUN_H
/* Corn Kidz 64 v1.029 PlayerCtrl::Run PRELUDE ONLY (0x06000294).
 * Does not implement the large gameplay-state dispatch, Unity objects, input
 * acquisition, or the N64 frontend. External systems consume entry effects. */
#include <stdbool.h>
#include <stdint.h>

enum CK64RunEntryEffect {
    CK64_RUN_STOP_DUST        = 1u << 0,
    CK64_RUN_STOP_RIPPLE      = 1u << 1,
    CK64_RUN_STOP_SPLASH      = 1u << 2,
    CK64_RUN_HIDE_FISTS       = 1u << 3,
    CK64_RUN_HIDE_PROMPTS     = 1u << 4,
    CK64_RUN_RESET_COLLIDER   = 1u << 5,
    CK64_RUN_RESTORE_LAYERS   = 1u << 6,
    CK64_RUN_HIDE_DIG_MOUND   = 1u << 7,
    CK64_RUN_STOP_SFX_53     = 1u << 8,
    CK64_RUN_STOP_SFX_55     = 1u << 9
};
#define CK64_RUN_ENTRY_BASE_EFFECTS ( \
    CK64_RUN_STOP_DUST | CK64_RUN_STOP_RIPPLE | CK64_RUN_STOP_SPLASH | \
    CK64_RUN_HIDE_FISTS | CK64_RUN_HIDE_PROMPTS | CK64_RUN_RESET_COLLIDER | \
    CK64_RUN_RESTORE_LAYERS | CK64_RUN_HIDE_DIG_MOUND )

typedef struct CK64RunInput {
    bool a_down, a_up, a_hold, x_down, x_hold;
    bool b_down, b_hold, y_down, y_hold;
    bool rb_hold, lb_hold, lb_down;
    float ctrl_x, ctrl_y;
    int32_t a_timer, rt_timer, lt_timer;
} CK64RunInput;

typedef struct CK64RunPrelude {
    int32_t pl_state, change_state, last_state;
    int32_t pl_timer, pl_timer2, pl_timer3, pl_counter, pl_switch;
    float pl_float;
    bool no_hurt, no_collision, no_look, wall_attached;
    bool no_control, underwater;
    int32_t camera_type;
    float rumble_l, rumble_r, rumble_fade, rumble_timer;
    CK64RunInput input;
} CK64RunPrelude;

/* Phase 16 source-grounded subset of Run's early control flow. The events
 * describe Unity side effects; they are NOT executed by this PAL module.
 * The input struct is filled by a separate backend on enabled frames.
 * On error state and effect output are unchanged. */
bool ck64_player_run_init(CK64RunPrelude *s);
bool ck64_player_run_prelude(CK64RunPrelude *s, uint32_t *entry_effects);
#endif
