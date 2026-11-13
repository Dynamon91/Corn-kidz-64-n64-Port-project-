#include "ck64_playercore.h"
#include <math.h>
bool ck64_playercore_init(CK64PlayerCore *p) {
    if (!p) return false;
    *p=(CK64PlayerCore){0}; p->camera_type=1; return true;
}
bool ck64_playercore_gravity(CK64PlayerCore *p,float decrement,float terminal_speed) {
    /* The IL's comparison is unordered, so the PAL rejects nonfinite inputs
     * instead of propagating nonfinite state. Finite values follow IL exactly. */
    if (!p || !isfinite(p->y_velocity) || !isfinite(decrement) ||
        !isfinite(terminal_speed)) return false;
    float y=p->y_velocity-decrement;
    if (!isfinite(y)) return false;
    if (y < -terminal_speed) y=-terminal_speed;
    if (p->on_floor) y=-0.1f;
    p->y_velocity=y; return true;
}
bool ck64_playercore_reset(CK64PlayerCore *p) {
    if (!p) return false;
    /* Original ResetPl IL resets only these fields. yVel is unchanged. */
    p->pl_state=0; p->no_control=false; p->camera_type=1;
    p->z_mov=0.0f; p->pl_velocity=(CK64PlayerVec3){0,0,0};
    return true;
}
CK64PlayerVec3 ck64_playercore_zxpos(CK64PlayerVec3 in) {
    return (CK64PlayerVec3){in.x,0.0f,in.z};
}
bool ck64_playercore_look_disable(CK64PlayerCore *p) {
    if (!p) return false; p->no_look=true; return true;
}
bool ck64_playercore_look_enable(CK64PlayerCore *p) {
    if (!p) return false; p->no_look=false; return true;
}
bool ck64_playercore_attack_disable(CK64PlayerCore *p) {
    if (!p) return false; p->no_attack=true; return true;
}
bool ck64_playercore_camera_shake(CK64PlayerCore *p,float amount) {
    if (!p || !isfinite(amount)) return false;
    p->camera_shake_ud=amount; return true;
}
bool ck64_playercore_set_fall_respawn(CK64PlayerCore *p,float amount) {
    if (!p || !isfinite(amount)) return false;
    p->fall_respawn_time=amount; return true;
}
