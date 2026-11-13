#ifndef CK64_WORLD3D_H
#define CK64_WORLD3D_H
/* Portable Phase 8 navigation PROTOTYPE, not reverse engineered PlayerCtrl.
 * Origin y=0 is a flat test floor. World units are arbitrary, not Unity units.
 * Fixed 30Hz stepping ensures repeatable host/N64 behavior.
 */
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct CK64Vec3 {
    float x, y, z;
} CK64Vec3;

typedef struct CK64WorldInput {
    float stick_x, stick_y;   /* -1..1, stick_y forward is positive */
    float camera_turn;        /* -1..1, negative left, positive right */
    bool jump;                /* rising-edge command, ignored in air */
    bool reset;
} CK64WorldInput;

typedef struct CK64World3D {
    CK64Vec3 position;
    CK64Vec3 velocity;
    float camera_yaw;
    bool grounded;
    unsigned total_steps;
} CK64World3D;

typedef struct CK64ScreenPoint { float x, y; } CK64ScreenPoint;

void ck64_world3d_reset(CK64World3D *world);
/* Return false on invalid pointers, nonfinite analog inputs, or counter overflow.
 * Does not modify the world on invalid input. */
bool ck64_world3d_step(CK64World3D *world, CK64WorldInput input);
/* Perspective camera always aims at avatar chest, at 320x240. Near clipping
 * is conservative: endpoints behind the near plane are rejected, not sliced. */
bool ck64_world3d_project(const CK64World3D *world, CK64Vec3 point,
                          CK64ScreenPoint *screen);
/* 2D viewport clipping for graphics_draw_line; useful to avoid out of bounds
 * writes in a software 3D wireframe diagnostic. */
bool ck64_world3d_clip_line(CK64ScreenPoint *a, CK64ScreenPoint *b);
#ifdef __cplusplus
}
#endif
#endif
