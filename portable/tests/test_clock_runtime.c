#include "ck64_clock.h"
#include "ck64_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

typedef struct TestBackend {
    char scene[CK64_SCENE_NAME_CAPACITY];
    int save_count, load_count, fade_count;
    bool reject_load;
} TestBackend;
static const char *active_scene(void *ctx) {
    return ((TestBackend *)ctx)->scene;
}
static bool save_ram(void *ctx) {
    ++((TestBackend *)ctx)->save_count;
    return true;
}
static bool load_scene(void *ctx, const char *name) {
    TestBackend *b = ctx;
    ++b->load_count;
    if (b->reject_load) return false;
    if (strlen(name) >= sizeof b->scene) return false;
    strcpy(b->scene, name);
    return true;
}
static bool fade_out(void *ctx) {
    ++((TestBackend *)ctx)->fade_count;
    return true;
}
static CK64Backend backend(TestBackend *b) {
    CK64Backend adapter = {b, active_scene, save_ram, load_scene, fade_out};
    return adapter;
}
static void setup(CK64Runtime *runtime, TestBackend *b) {
    memset(b, 0, sizeof *b);
    strcpy(b->scene, "diagnostic");
    assert(ck64_runtime_init(runtime, backend(b), 3000) == CK64_RUNTIME_OK);
}
static void test_clock_validation(void) {
    CK64Clock c;
    CK64ClockResult res = {77, true};
    assert(ck64_clock_init(NULL, 3000) == CK64_CLOCK_BAD_ARGUMENT);
    assert(ck64_clock_init(&c, 0) == CK64_CLOCK_BAD_ARGUMENT);
    assert(ck64_clock_init(&c, UINT64_MAX) == CK64_CLOCK_BAD_ARGUMENT);
    assert(ck64_clock_init(&c, 3000) == CK64_CLOCK_OK);
    assert(ck64_clock_advance(NULL, 0, &res) == CK64_CLOCK_BAD_ARGUMENT);
    assert(ck64_clock_advance(&c, 0, NULL) == CK64_CLOCK_BAD_ARGUMENT);
    assert(ck64_clock_advance(&c, 250, &res) == CK64_CLOCK_OK);
    assert(res.updates == 0 && !res.dropped_time);
    assert(ck64_clock_advance(&c, 200, &res) == CK64_CLOCK_BACKWARDS);
    assert(c.last_ticks == 250);
    assert(res.updates == 0 && !res.dropped_time);
}
static void test_60hz_updates_30hz(void) {
    CK64Clock c;
    CK64ClockResult res;
    unsigned updates = 0;
    assert(ck64_clock_init(&c, 3000) == CK64_CLOCK_OK);
    for (uint64_t i = 0; i <= 60; ++i) {
        assert(ck64_clock_advance(&c, i * 50, &res) == CK64_CLOCK_OK);
        updates += res.updates;
    }
    assert(updates == 30 && c.phase == 0);
}
static void test_50hz_updates_30hz(void) {
    CK64Clock c;
    CK64ClockResult res;
    unsigned updates = 0;
    assert(ck64_clock_init(&c, 3000) == CK64_CLOCK_OK);
    for (uint64_t i = 0; i <= 50; ++i) {
        assert(ck64_clock_advance(&c, i * 60, &res) == CK64_CLOCK_OK);
        updates += res.updates;
    }
    assert(updates == 30 && c.phase == 0);
}
static void test_suspension_clamped(void) {
    CK64Clock c;
    CK64ClockResult res;
    assert(ck64_clock_init(&c, 3000) == CK64_CLOCK_OK);
    assert(ck64_clock_advance(&c, 0, &res) == CK64_CLOCK_OK);
    assert(ck64_clock_advance(&c, 300000, &res) == CK64_CLOCK_OK);
    assert(res.updates == CK64_CLOCK_MAX_STEPS && res.dropped_time);
    assert(c.phase < c.ticks_per_second);
    assert(ck64_clock_advance(&c, 300000, &res) == CK64_CLOCK_OK);
    assert(res.updates == 0 && !res.dropped_time);
}
static void test_max_time_no_overflow(void) {
    CK64Clock c;
    CK64ClockResult res;
    const uint64_t tps = UINT64_MAX / CK64_CLOCK_UPDATE_HZ;
    assert(ck64_clock_init(&c, tps) == CK64_CLOCK_OK);
    assert(ck64_clock_advance(&c, 0, &res) == CK64_CLOCK_OK);
    assert(ck64_clock_advance(&c, tps/4, &res) == CK64_CLOCK_OK);
    assert(res.updates == CK64_CLOCK_MAX_STEPS && res.dropped_time);
    assert(c.phase < tps);
}
static void test_runtime_rate_and_fade(void) {
    CK64Runtime runtime;
    TestBackend b;
    CK64ClockResult res;
    setup(&runtime, &b);
    assert(ck64_runtime_frame(&runtime, 0, 1, &res) == CK64_RUNTIME_OK);
    assert(res.updates == 0 && runtime.fixed_updates == 0);
    assert(ck64_runtime_frame(&runtime, 50, 1, &res) == CK64_RUNTIME_OK);
    assert(runtime.fixed_updates == 0);
    assert(ck64_runtime_frame(&runtime, 100, 1, &res) == CK64_RUNTIME_OK);
    assert(runtime.fixed_updates == 1 && runtime.flow.scene_timer == 1);
    assert(ck64_fade_to_scene(&runtime.flow, "test_room", 3) == CK64_OK);
    assert(b.fade_count == 1 && runtime.flow.go_to_door == 3);
    for (unsigned i=1; i<=50; ++i) {
        assert(ck64_runtime_frame(&runtime, 100 + (uint64_t)i*100, 1, &res)
               == CK64_RUNTIME_OK);
        assert(b.load_count == (i==50 ? 1 : 0));
    }
    assert(strcmp(b.scene, "test_room") == 0);
    assert(runtime.flow.scene_timer == 0 && !runtime.flow.fading);
    assert(runtime.fixed_updates == 51);
    assert(b.save_count == 1);
}
static void test_runtime_music_and_errors(void) {
    CK64Runtime runtime;
    TestBackend b;
    CK64ClockResult res;
    setup(&runtime, &b);
    runtime.music.sub_track = 1;
    assert(ck64_runtime_frame(&runtime, 0, 1, &res) == CK64_RUNTIME_OK);
    for (unsigned i=1; i<=25; ++i)
        assert(ck64_runtime_frame(&runtime, (uint64_t)i * 100, 1, &res)
               == CK64_RUNTIME_OK);
    assert(runtime.music.channel[1].volume > 0.999f);
    assert(runtime.music.channel[0].volume == 0.0f);

    /* Make auto-switch active with no associated clip table -> explicit error.
       Source scene/mixer must supply valid clip count before calling. */
    setup(&runtime, &b);
    runtime.music.auto_switch = true;
    runtime.flow.next_track = 1; /* force fade to zero before switch */
    assert(ck64_runtime_frame(&runtime, 0, 0, &res) == CK64_RUNTIME_OK);
    assert(ck64_runtime_frame(&runtime, 100, 0, &res)
           == CK64_RUNTIME_MUSIC_ERROR);
    assert(runtime.music_error == CK64_MUSIC_INVALID_TRACK);
    assert(runtime.fixed_updates == 0);
    assert(runtime.flow.scene_timer == 1); /* no rollback across systems */
}
static void test_runtime_reject_invalid_callbacks(void) {
    CK64Runtime runtime;
    CK64Backend empty = {0};
    assert(ck64_runtime_init(NULL, empty, 3000) == CK64_RUNTIME_BAD_ARGUMENT);
    assert(ck64_runtime_init(&runtime, empty, 3000) == CK64_RUNTIME_BAD_ARGUMENT);
}
int main(void) {
    test_clock_validation();
    test_60hz_updates_30hz();
    test_50hz_updates_30hz();
    test_suspension_clamped();
    test_max_time_no_overflow();
    test_runtime_rate_and_fade();
    test_runtime_music_and_errors();
    test_runtime_reject_invalid_callbacks();
    puts("8 clock/runtime scenario tests passed");
    return 0;
}
