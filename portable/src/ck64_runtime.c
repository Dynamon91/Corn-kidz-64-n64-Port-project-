#include "ck64_runtime.h"
#include <string.h>
#include <stdint.h>

CK64RuntimeStatus ck64_runtime_init(CK64Runtime *runtime, CK64Backend backend,
                                    uint64_t ticks_per_second) {
    if (!runtime || !backend.get_active_scene || !backend.load_scene ||
        !backend.save_game || !backend.transition_out)
        return CK64_RUNTIME_BAD_ARGUMENT;
    memset(runtime, 0, sizeof(*runtime));
    if (ck64_clock_init(&runtime->clock, ticks_per_second) != CK64_CLOCK_OK)
        return CK64_RUNTIME_BAD_ARGUMENT;
    ck64_gameflow_init(&runtime->flow, backend);
    ck64_music_init(&runtime->music);
    return CK64_RUNTIME_OK;
}

CK64RuntimeStatus ck64_runtime_frame(CK64Runtime *runtime, uint64_t now,
                                     size_t track_count,
                                     CK64ClockResult *frame_result) {
    CK64ClockResult result;
    if (!runtime || !frame_result) return CK64_RUNTIME_BAD_ARGUMENT;
    runtime->clock_error = ck64_clock_advance(&runtime->clock, now, &result);
    if (runtime->clock_error != CK64_CLOCK_OK)
        return CK64_RUNTIME_CLOCK_ERROR;
    *frame_result = result;
    for (uint32_t i = 0; i < result.updates; ++i) {
        if (runtime->fixed_updates == UINT64_MAX)
            return CK64_RUNTIME_BAD_ARGUMENT;
        runtime->flow_error = ck64_gameflow_update_counters(&runtime->flow);
        if (runtime->flow_error != CK64_OK) return CK64_RUNTIME_FLOW_ERROR;
        /* Synchronize the GameCtrl-owned nextTrack field. Track clip tables
         * and their real state will be recovered from Unity scene metadata. */
        runtime->music.next_track = runtime->flow.next_track;
        runtime->music_error = ck64_music_step(&runtime->music, track_count,
                                               &runtime->last_music_events);
        if (runtime->music_error != CK64_MUSIC_OK) return CK64_RUNTIME_MUSIC_ERROR;
        if (runtime->flow.transition_active) {
            runtime->flow_error = ck64_transition_tick(&runtime->flow);
            if (runtime->flow_error != CK64_OK) return CK64_RUNTIME_FLOW_ERROR;
        }
        ++runtime->fixed_updates;
    }
    return CK64_RUNTIME_OK;
}
