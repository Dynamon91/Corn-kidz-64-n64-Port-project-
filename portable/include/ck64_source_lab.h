#ifndef CK64_SOURCE_LAB_H
#define CK64_SOURCE_LAB_H
/* Experimental Phase 19 adapter: ORIGINAL reconstructed PlayerCtrl functions
 * drive a synthetic room. Camera, geometry, wall contacts and analog mapping
 * are all NEW TEST ADAPTERS, NOT taken from Corn Kidz 64. */
#include "ck64_player_state0.h"
#include "ck64_player_footer.h"
#include "ck64_world3d.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct CK64SourceLabInput {
    float stick_x, stick_y, camera_turn;
    bool jump_press, jump_release, reset;
} CK64SourceLabInput;

typedef struct CK64SourceLab {
    CK64World3D world;        /* wireframe render and test-room collision */
    CK64PlayerMotion motion;  /* reconstructed MovPl */
    CK64State0 state0;        /* reconstructed Run state-0 */
    CK64PlayerRunFooter floor_history;
    CK64State0Result last_decision;
    unsigned jumps, landings;
    bool initialized;
} CK64SourceLab;

bool ck64_source_lab_init(CK64SourceLab *lab);
/* One 30 Hz *diagnostic* step. No real Unity level/collision/animation data.
 * Fails atomically on bad input or internal numerical failure. */
bool ck64_source_lab_step(CK64SourceLab *lab, CK64SourceLabInput in);
#endif
