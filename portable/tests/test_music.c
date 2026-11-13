#include "ck64_music.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool near(float a, float b) {return fabsf(a-b)<0.00002f;}

static void test_initialization(void) {
    CK64MusicState s;
    ck64_music_init(&s);
    assert(s.music_enabled && near(s.music_speed,1.0f));
    assert(near(s.channel[0].pitch,1.0f));
    assert(near(s.channel[1].pitch,1.0f));
}

static void test_main_fades_in_and_out(void) {
    CK64MusicState s;
    ck64_music_init(&s);
    assert(ck64_music_step(&s,5,NULL)==CK64_MUSIC_OK);
    assert(near(s.channel[0].volume,0.04f));
    s.channel[0].volume=1.0f; s.next_track=1;
    assert(ck64_music_step(&s,5,NULL)==CK64_MUSIC_OK);
    assert(near(s.channel[0].volume,0.96f));
}

static void test_crossfade_switch_exact_zero(void) {
    CK64MusicState s;CK64MusicEvents e;
    ck64_music_init(&s);
    s.channel[0].volume=1.0f;s.current_track=1;s.next_track=2;
    s.auto_switch=true;
    for(int i=0;i<24;i++) {
        assert(ck64_music_step(&s,4,&e)==CK64_MUSIC_OK);
        assert(!e.main_play && s.current_track==1);
    }
    assert(near(s.channel[0].volume,0.04f));
    assert(ck64_music_step(&s,4,&e)==CK64_MUSIC_OK);
    assert(e.main_play && e.main_clip_changed && e.selected_track==2);
    assert(s.channel[0].playing && s.channel[0].clip_index==2);
    assert(s.current_track==2 && !s.auto_switch);
    assert(near(s.channel[0].volume,0.0f));
    assert(ck64_music_step(&s,4,&e)==CK64_MUSIC_OK);
    assert(!e.main_play && near(s.channel[0].volume,0.04f));
}

static void test_no_auto_switch_when_flag_false(void) {
    CK64MusicState s;CK64MusicEvents e;
    ck64_music_init(&s);
    s.current_track=1;s.next_track=2;s.channel[0].volume=0.0f;
    assert(ck64_music_step(&s,3,&e)==CK64_MUSIC_OK);
    assert(s.current_track==1 && !e.main_clip_changed);
}

static void test_subtrack_fade_and_stop(void) {
    CK64MusicState s;CK64MusicEvents e;
    ck64_music_init(&s);
    s.sub_track=4;s.channel[1].playing=true;
    assert(ck64_music_step(&s,2,&e)==CK64_MUSIC_OK);
    assert(near(s.channel[1].volume,0.04f) && !e.sub_stop);
    s.sub_track=0;
    assert(ck64_music_step(&s,2,&e)==CK64_MUSIC_OK);
    assert(near(s.channel[1].volume,0.0f) && e.sub_stop);
    assert(!s.channel[1].playing);
}

static void test_subtrack_inactive_not_playing_keeps_volume(void) {
    CK64MusicState s;
    ck64_music_init(&s);
    s.channel[1].volume=0.4f;s.channel[1].playing=false;
    assert(ck64_music_step(&s,1,NULL)==CK64_MUSIC_OK);
    assert(near(s.channel[1].volume,0.4f));
}

static void test_pitch_match_no_change_when_subactive(void) {
    CK64MusicState s;
    ck64_music_init(&s);
    s.sub_track=1;s.music_speed=0.6f;
    s.channel[0].pitch=1.0f;s.channel[1].pitch=0.6f;
    assert(ck64_music_step(&s,1,NULL)==CK64_MUSIC_OK);
    /* Source condition checks channel[1].pitch ONLY. */
    assert(near(s.channel[0].pitch,1.0f));
}

static void test_pitch_dual_adjust_and_restore(void) {
    CK64MusicState s;
    ck64_music_init(&s);
    s.sub_track=1;s.music_speed=0.5f;
    assert(ck64_music_step(&s,1,NULL)==CK64_MUSIC_OK);
    assert(near(s.channel[0].pitch,0.98f));
    assert(near(s.channel[1].pitch,0.98f));
    s.sub_track=0;
    assert(ck64_music_step(&s,1,NULL)==CK64_MUSIC_OK);
    assert(near(s.channel[0].pitch,0.98f)); /* .96 then +.02 */
    assert(near(s.channel[1].pitch,0.96f));
    assert(near(s.music_speed,1.0f));
}

static void test_muting_order_and_recovery(void) {
    CK64MusicState s;CK64MusicEvents e;
    ck64_music_init(&s);
    s.music_enabled=false;s.sub_track=1;
    s.channel[0].volume=0.8f;s.channel[1].playing=true;
    assert(ck64_music_step(&s,1,&e)==CK64_MUSIC_OK);
    assert(near(s.channel[0].volume,0.0f));
    assert(near(s.channel[1].volume,0.0f) && s.channel[1].playing);
    s.music_enabled=true;s.sub_track=0;
    assert(ck64_music_step(&s,1,&e)==CK64_MUSIC_OK);
    assert(near(s.channel[0].volume,0.04f));
    assert(e.sub_stop && !s.channel[1].playing);
}

static void test_invalid_track_rollback(void) {
    CK64MusicState s,before;CK64MusicEvents e;
    ck64_music_init(&s);
    s.current_track=0;s.next_track=9;s.auto_switch=true;
    s.channel[0].volume=0.02f;
    before=s;
    e.main_play=true;e.main_clip_changed=true;
    assert(ck64_music_step(&s,4,&e)==CK64_MUSIC_INVALID_TRACK);
    assert(memcmp(&s,&before,sizeof(s))==0);
    assert(e.main_play && e.main_clip_changed); /* Previous event unchanged. */
}

static void test_invalid_float_rollback(void) {
    CK64MusicState s,before;
    ck64_music_init(&s);
    s.music_speed=NAN; before=s;
    assert(ck64_music_step(&s,2,NULL)==CK64_MUSIC_INVALID_FLOAT);
    assert(memcmp(&s,&before,sizeof(s))==0);
    s.music_speed=1.0f;s.channel[0].pitch=INFINITY;before=s;
    assert(ck64_music_step(&s,2,NULL)==CK64_MUSIC_INVALID_FLOAT);
    assert(memcmp(&s,&before,sizeof(s))==0);
}

static void test_null_state(void) {
    CK64MusicEvents e={0};
    ck64_music_init(NULL);
    assert(ck64_music_step(NULL,2,&e)==CK64_MUSIC_INVALID_ARGUMENT);
}
int main(void) {
    test_initialization();test_main_fades_in_and_out();
    test_crossfade_switch_exact_zero();test_no_auto_switch_when_flag_false();
    test_subtrack_fade_and_stop();test_subtrack_inactive_not_playing_keeps_volume();
    test_pitch_match_no_change_when_subactive();test_pitch_dual_adjust_and_restore();
    test_muting_order_and_recovery();test_invalid_track_rollback();
    test_invalid_float_rollback();test_null_state();
    puts("12 music state-machine tests passed");
    return 0;
}
