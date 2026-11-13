#include "ck64_player_run.h"
#include <limits.h>
#include <math.h>

static float move_toward_zero(float x, float delta) {
    if (fabsf(x) <= delta) return 0.0f;
    return x > 0.0f ? x - delta : x + delta;
}

bool ck64_player_run_init(CK64RunPrelude *s) {
    if (!s) return false;
    *s = (CK64RunPrelude){0};
    s->camera_type = 1;
    return true;
}

bool ck64_player_run_prelude(CK64RunPrelude *s, uint32_t *entry_effects) {
    if (!s || !entry_effects ||
        !isfinite(s->rumble_l) || !isfinite(s->rumble_r) ||
        !isfinite(s->rumble_fade) || !isfinite(s->rumble_timer) ||
        !isfinite(s->pl_float) || s->rumble_fade < 0.0f ||
        s->input.a_timer < 0 ||
        (!s->no_control && s->input.a_hold && s->input.a_timer == INT32_MAX))
        return false;

    CK64RunPrelude n = *s;
    uint32_t effects = 0;

    /* Original Run IL 0x0000-0x015e: this is an ENTRY edge, not an every-frame
     * reset. Unity game objects and physics layer calls are deferred to frontend. */
    if (n.change_state != n.pl_state) {
        n.pl_timer = 0;
        n.pl_timer2 = 0;
        n.pl_timer3 = 0;
        n.pl_counter = 0;
        n.pl_float = 0.0f;
        n.no_hurt = false;
        n.last_state = n.change_state;
        n.change_state = n.pl_state;
        n.no_collision = false;
        n.no_look = false;
        n.wall_attached = false;
        effects = CK64_RUN_ENTRY_BASE_EFFECTS;
        /* Original uses the PREVIOUS changeState, not the new plState. */
        if (n.last_state == 46) effects |= CK64_RUN_STOP_SFX_53;
        if (n.last_state == 45) effects |= CK64_RUN_STOP_SFX_55;
    }
    /* Run IL 0x015e: timer 1 clears plSwitch (not timer 0). */
    if (n.pl_timer == 1) n.pl_switch = 0;

    /* Run IL 0x019d-0x01f6: timer counts down in frame units; only after
     * expiry do the left/right motor strengths converge toward zero. */
    if (n.rumble_timer > 0.0f) {
        n.rumble_timer -= 1.0f;
    } else {
        n.rumble_l = move_toward_zero(n.rumble_l, n.rumble_fade);
        n.rumble_r = move_toward_zero(n.rumble_r, n.rumble_fade);
    }

    /* Input acquisition is Unity-specific and must happen upstream. Run IL
     * 0x06f3-0x0773: while bNoCtrl, only THESE values are zeroed; Bhold,
     * RBhold, LBhold, LBdown etc. are NOT cleared. */
    if (n.no_control) {
        n.input.a_down = false;
        n.input.a_up = false;
        n.input.a_hold = false;
        n.input.x_down = false;
        n.input.x_hold = false;
        n.input.b_down = false;
        n.input.y_down = false;
        n.input.y_hold = false;
        n.input.rt_timer = 0;
        n.input.lt_timer = 0;
        n.input.ctrl_x = 0.0f;
        n.input.ctrl_y = 0.0f;
        n.input.a_timer = 0;
    } else {
        /* Run IL 0x06f3: Ahold increments Atimer, else clears it. */
        if (n.input.a_hold) n.input.a_timer++;
        else n.input.a_timer = 0;
    }

    /* Run IL 0x077f-0x07bb. 4 is a fixed-camera exception, 18 is a
     * particular player state; these are original IDs, not new enums. */
    if (n.underwater && n.camera_type != 4) n.camera_type = 2;
    else if (n.pl_state == 18) n.camera_type = 3;
    else if (n.camera_type != 4 && n.camera_type != 0) n.camera_type = 1;

    if (!isfinite(n.rumble_l) || !isfinite(n.rumble_r) ||
        !isfinite(n.rumble_timer)) return false;
    *s = n;
    *entry_effects = effects;
    return true;
}
