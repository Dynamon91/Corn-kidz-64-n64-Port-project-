#ifndef CK64_RUNTIME_H
#define CK64_RUNTIME_H
/* Deterministic GameCtrl subset orchestration. No hardware or game assets. */
#include "ck64_clock.h"
#include "ck64_gameflow.h"
#include "ck64_music.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum CK64RuntimeStatus {
    CK64_RUNTIME_OK = 0,
    CK64_RUNTIME_BAD_ARGUMENT,
    CK64_RUNTIME_CLOCK_ERROR,
    CK64_RUNTIME_FLOW_ERROR,
    CK64_RUNTIME_MUSIC_ERROR
} CK64RuntimeStatus;

typedef struct CK64Runtime {
    CK64GameFlow flow;
    CK64MusicState music;
    CK64Clock clock;
    uint64_t fixed_updates;
    CK64ClockStatus clock_error;
    CK64Status flow_error;
    CK64MusicStatus music_error;
    CK64MusicEvents last_music_events;
} CK64Runtime;

CK64RuntimeStatus ck64_runtime_init(CK64Runtime *runtime, CK64Backend backend,
                                    uint64_t ticks_per_second);
/* On each VI render iteration, advance the clock, then invoke a fixed number
 * of GameCtrl.Update(counter subset), MusicFade and transition coroutine ticks.
 * track_count refers to the external per-scene music clip table; the runtime
 * cannot play sound until a real audio backend is written.
 * A non-OK status means this frame stopped early; no silent error suppression.
 */
CK64RuntimeStatus ck64_runtime_frame(CK64Runtime *runtime, uint64_t now,
                                     size_t track_count,
                                     CK64ClockResult *frame_result);
#ifdef __cplusplus
}
#endif
#endif
