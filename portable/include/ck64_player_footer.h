#ifndef CK64_PLAYER_FOOTER_H
#define CK64_PLAYER_FOOTER_H
/* Source-derived PlayerCtrl::Run footer, v1.029 IL 75849..76018.
 * Does not implement collision detection or full gameplay. */
#include <stdint.h>
#include <stdbool.h>
typedef struct CK64PlayerRunFooter {
    int32_t pl_timer, pl_timer2, pl_timer3, on_floor_timer, pl_state;
    bool on_floor, no_air, last_wall_object_present;
} CK64PlayerRunFooter;
typedef struct CK64PlayerRunFooterResult {
    bool landing_edge, left_floor, cleared_wall;
} CK64PlayerRunFooterResult;
/* Source rules; C11 overflow guard makes failure atomic.
 * Edge flags are synthetic diagnostics, not original game events. */
bool ck64_player_run_footer_step(CK64PlayerRunFooter *s,
                                 CK64PlayerRunFooterResult *r);
#endif
