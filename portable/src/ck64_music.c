#include "ck64_music.h"
#include <math.h>
#include <string.h>

/* UnityEngine.Mathf.MoveTowards: no time scaling. */
static float approach(float current, float target, float step) {
    float delta = target - current;
    if (delta > step) return current + step;
    if (delta < -step) return current - step;
    return target;
}

void ck64_music_init(CK64MusicState *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->channel[0].pitch = 1.0f;
    state->channel[1].pitch = 1.0f;
    state->music_speed = 1.0f;
    state->music_enabled = true;
}

CK64MusicStatus ck64_music_step(CK64MusicState *state, size_t track_count,
                               CK64MusicEvents *events) {
    CK64MusicState next;
    CK64MusicEvents generated = {0};
    CK64MusicChannel *main, *sub;
    float new_pitch;
    if (!state) return CK64_MUSIC_INVALID_ARGUMENT;
    /* A malformed save/adapter must never pass NaNs/infinities to an N64
     * backend. Unlike Unity, invalid input results in a clean status. */
    for (size_t i = 0; i < 2; ++i) {
        if (!isfinite(state->channel[i].volume) ||
            !isfinite(state->channel[i].pitch))
            return CK64_MUSIC_INVALID_FLOAT;
    }
    if (!isfinite(state->music_speed)) return CK64_MUSIC_INVALID_FLOAT;

    next = *state;
    main = &next.channel[0];
    sub = &next.channel[1];

    /* MusicFade IL offsets 0–106: fade main out when nextTrack !=
     * currentTrack OR a non-zero subTrack is active. */
    main->volume = approach(main->volume,
                            (next.current_track != next.next_track || next.sub_track)
                                ? 0.0f : 1.0f,
                            0.04f);

    /* IL 106–192: switch main clip at exactly zero volume, then Play.
     * Check bounds before publishing state to avoid managed array OOB. */
    if (next.auto_switch && main->volume == 0.0f) {
        if (next.next_track < 0 || (size_t)next.next_track >= track_count)
            return CK64_MUSIC_INVALID_TRACK;
        next.auto_switch = false;
        next.current_track = next.next_track;
        main->clip_index = next.current_track;
        main->playing = true;
        generated.main_clip_changed = true;
        generated.main_play = true;
        generated.selected_track = next.current_track;
    }

    /* IL 192–355: fade secondary channel up for any non-zero subTrack,
     * otherwise fade down only if it is currently playing, then stop at 0. */
    if (next.sub_track) {
        sub->volume = approach(sub->volume, 1.0f, 0.04f);
    } else {
        if (sub->playing)
            sub->volume = approach(sub->volume, 0.0f, 0.04f);
        if (sub->volume == 0.0f && sub->playing) {
            sub->playing = false;
            generated.sub_stop = true;
        }
    }

    /* IL 355–460: note the unusual source condition checks ONLY sub.pitch
     * against musicSpeed, but if mismatched moves BOTH channel pitches. */
    if (sub->pitch != next.music_speed) {
        main->pitch = approach(main->pitch, next.music_speed, 0.02f);
        sub->pitch = approach(sub->pitch, next.music_speed, 0.02f);
    }

    /* IL 460–520: when no subTrack, reset requested speed to 1 then
     * separately approach main channel pitch back to 1. */
    if (!next.sub_track) {
        next.music_speed = 1.0f;
        new_pitch = approach(main->pitch, 1.0f, 0.02f);
        main->pitch = new_pitch;
    }

    /* IL 520–569: bMusic false hard-mutes both channels *after* fade logic.
     * Playback status itself is unaffected by this source branch. */
    if (!next.music_enabled) {
        main->volume = 0.0f;
        sub->volume = 0.0f;
    }
    /* Pure-state model is transactional. No N64 audio callbacks yet. */
    *state = next;
    if (events) *events = generated;
    return CK64_MUSIC_OK;
}
