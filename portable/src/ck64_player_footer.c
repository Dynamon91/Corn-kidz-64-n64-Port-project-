#include "ck64_player_footer.h"
#include <limits.h>
bool ck64_player_run_footer_step(CK64PlayerRunFooter *s,
                                 CK64PlayerRunFooterResult *r) {
    if (!s || !r || s->pl_timer==INT32_MAX || s->pl_timer2==INT32_MAX ||
        s->pl_timer3==INT32_MAX ||
        (s->on_floor && s->on_floor_timer==INT32_MAX) ||
        (!s->on_floor && s->on_floor_timer==INT32_MIN)) return false;
    CK64PlayerRunFooter n=*s;
    CK64PlayerRunFooterResult e={0};
    ++n.pl_timer; ++n.pl_timer2; ++n.pl_timer3; n.no_air=false;
    if (n.on_floor) {
        e.landing_edge=n.on_floor_timer<0;
        if (n.on_floor_timer<0) n.on_floor_timer=0;
        ++n.on_floor_timer;
    } else {
        e.left_floor=n.on_floor_timer>0;
        if (n.on_floor_timer>0) n.on_floor_timer=0;
        --n.on_floor_timer;
    }
    if (n.on_floor || n.pl_state==11 || n.pl_state==12 || n.pl_state==4) {
        e.cleared_wall=n.last_wall_object_present;
        n.last_wall_object_present=false;
    }
    *s=n; *r=e;
    return true;
}
