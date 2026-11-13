#ifndef CK64_CLOCK_H
#define CK64_CLOCK_H
/* Integer fixed-step scheduler. No Unix/Unity/N64 dependencies.
 * The original GameCtrl.Update/MusicFade are invoked per game update;
 * original Awake sets Unity targetFrameRate=30. A 30 Hz fixed-step loop
 * is a starting point, not proof that N64 frame pacing matches Unity.
 */
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum { CK64_CLOCK_UPDATE_HZ = 30, CK64_CLOCK_MAX_STEPS = 4 };

typedef enum CK64ClockStatus {
    CK64_CLOCK_OK = 0,
    CK64_CLOCK_BAD_ARGUMENT,
    CK64_CLOCK_BACKWARDS
} CK64ClockStatus;

typedef struct CK64Clock {
    uint64_t ticks_per_second;
    uint64_t last_ticks;
    uint64_t phase; /* scaled by UPDATE_HZ; invariant phase < tps */
    bool primed;
} CK64Clock;

typedef struct CK64ClockResult {
    uint32_t updates;
    bool dropped_time; /* long suspension or too many steps to catch up */
} CK64ClockResult;

/* A nonzero, reasonable hardware tick frequency is mandatory. */
CK64ClockStatus ck64_clock_init(CK64Clock *clock, uint64_t ticks_per_second);
/* Provide a monotonically increasing hardware tick value each render frame.
 * First call primes the clock and produces zero updates. PAL 50Hz / NTSC 60Hz
 * both generate an average of 30 updates per real second. On a long stall,
 * the scheduler limits catchup and discards overdue whole updates to avoid
 * a spiral of death (explicitly different from Unity's variable timing).
 * On error the clock and output remain unchanged.
 */
CK64ClockStatus ck64_clock_advance(CK64Clock *clock, uint64_t now,
                                  CK64ClockResult *result);
#ifdef __cplusplus
}
#endif
#endif
