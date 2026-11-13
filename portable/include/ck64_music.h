#ifndef CK64_MUSIC_H
#define CK64_MUSIC_H
/* Corn Kidz 64 GameCtrl.MusicFade — two-channel, backend-free C11 model.
 * Source mapping: reports/gamectrl_il.json (GameCtrl::MusicFade).
 * Does NOT decode/stream original audio or constitute an N64 audio backend.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CK64MusicStatus {
    CK64_MUSIC_OK = 0,
    CK64_MUSIC_INVALID_ARGUMENT,
    CK64_MUSIC_INVALID_TRACK,
    CK64_MUSIC_INVALID_FLOAT
} CK64MusicStatus;

typedef struct CK64MusicChannel {
    float volume;
    float pitch;
    bool playing;
    int32_t clip_index; /* Index into an externally owned original track table. */
} CK64MusicChannel;

typedef struct CK64MusicState {
    CK64MusicChannel channel[2];
    int32_t current_track;
    int32_t next_track;
    int32_t sub_track; /* Original branch tests only zero/nonzero. */
    float music_speed;
    bool auto_switch;
    bool music_enabled; /* Data.bMusic */
} CK64MusicState;

typedef struct CK64MusicEvents {
    bool main_clip_changed;
    bool main_play;
    bool sub_stop;
    int32_t selected_track; /* Meaningful only when main_clip_changed. */
} CK64MusicEvents;

/* Deterministic *provisional* initialization for the portable runtime.
 * Unity scene-serialized source defaults have not been recovered. The caller
 * must overwrite fields with loaded scene/game state before its first tick.
 */
void ck64_music_init(CK64MusicState *state);

/* Simulates exactly ONE GameCtrl.MusicFade call (invoked from Update).
 * Step sizes are per source Update, NOT per second: 0.04 volume, 0.02 pitch.
 * On a track switch the caller must map selected_track to an N64 clip and
 * enact the main_play/sub_stop events on its own audio backend.
 * No external side-effects occur here. On error, state is unchanged.
 */
CK64MusicStatus ck64_music_step(CK64MusicState *state, size_t track_count,
                               CK64MusicEvents *events);

#ifdef __cplusplus
}
#endif
#endif
