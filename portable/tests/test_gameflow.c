#include "ck64_gameflow.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

typedef struct FakeBackend {
    char scene[CK64_SCENE_NAME_CAPACITY];
    char loaded[CK64_SCENE_NAME_CAPACITY];
    unsigned save_calls, load_calls, out_calls;
    bool fail_save, fail_load, fail_out;
} FakeBackend;
static const char *active(void *v) {return ((FakeBackend*)v)->scene;}
static bool save(void *v) {
    FakeBackend *b=(FakeBackend*)v; ++b->save_calls;return !b->fail_save;
}
static bool load(void *v,const char *scene) {
    FakeBackend *b=(FakeBackend*)v; ++b->load_calls;
    if(b->fail_load) return false;
    assert(strlen(scene)<sizeof(b->scene));
    strcpy(b->loaded,scene);strcpy(b->scene,scene);return true;
}
static bool out(void *v) {
    FakeBackend *b=(FakeBackend*)v; ++b->out_calls;return !b->fail_out;
}
static CK64Backend make_backend(FakeBackend *b) {
    CK64Backend x={b,active,save,load,out};return x;
}
static void setup(CK64GameFlow *g, FakeBackend *b) {
    memset(b,0,sizeof(*b));strcpy(b->scene,"field");
    ck64_gameflow_init(g,make_backend(b));
}
static void test_defaults(void) {
    FakeBackend b;CK64GameFlow g;setup(&g,&b);
    assert(g.start_hp==5 && g.hp==5 && g.dp==16);
    assert(g.data.current_level==1 && g.data.max_hp==8);
    assert(g.level_thresholds[2]==70 && g.level_thresholds[7]==999);
    assert(g.level_thresholds[8]==0);
}
static void test_level_progression(void) {
    FakeBackend b;CK64GameFlow g;bool changed=true;setup(&g,&b);
    g.data.item_total=69;
    assert(ck64_set_current_level(&g,&changed)==CK64_OK);
    assert(!changed && g.data.current_level==1);
    g.data.item_total=140;
    assert(ck64_set_current_level(&g,&changed)==CK64_OK);
    assert(changed && g.data.current_level==2); /* only one per call */
    assert(ck64_set_current_level(&g,&changed)==CK64_OK);
    assert(changed && g.data.current_level==3);
    g.data.current_level=98;
    assert(ck64_set_current_level(&g,&changed)==CK64_PROGRESS_OUT_OF_BOUNDS);
}
static void test_volume(void) {
    FakeBackend b;CK64GameFlow g;float gain=0;setup(&g,&b);
    assert(ck64_set_volume(&g,75,&gain)==CK64_OK);
    assert(g.data.volume==75 && gain>0.749f && gain<0.751f);
    assert(ck64_set_volume(&g,120,&gain)==CK64_OK);
    assert(gain>1.19f); /* original has no clamp */
}
static void test_scene_load(void) {
    FakeBackend b;CK64GameFlow g;setup(&g,&b);g.scene_timer=42;g.fading=true;
    assert(ck64_load_scene(&g,"cavern")==CK64_OK);
    assert(b.save_calls==1 && b.load_calls==1);
    assert(!strcmp(g.last_scene,"field") && !strcmp(b.scene,"cavern"));
    assert(!g.fading && g.scene_timer==0);
    assert(ck64_load_scene(&g,"")==CK64_OK);
    assert(b.load_calls==2 && !strcmp(b.loaded,"cavern"));
    assert(!strcmp(g.last_scene,"field")); /* reload preserves lastScene */
    assert(ck64_set_checkpoint_scene(&g)==CK64_OK);
    assert(!strcmp(g.checkpoint_scene,"cavern"));
}
static void test_gameover_lastscene(void) {
    FakeBackend b;CK64GameFlow g;setup(&g,&b);
    strcpy(b.scene,"gameover");strcpy(g.last_scene,"main_field");
    assert(ck64_load_scene(&g,"next")==CK64_OK);
    assert(!strcmp(g.last_scene,"main_field"));
}
static void test_fade_exactly_50_ticks(void) {
    FakeBackend b;CK64GameFlow g;setup(&g,&b);
    g.check_point_door=7;
    assert(ck64_fade_to_scene(&g,"boss",0)==CK64_OK);
    assert(g.fading && g.transition_active && g.go_to_door==7);
    assert(b.out_calls==1 && b.load_calls==0);
    for(unsigned i=0;i<49;i++) {
        assert(ck64_transition_tick(&g)==CK64_OK);
        assert(b.load_calls==0);
    }
    assert(ck64_transition_tick(&g)==CK64_OK);
    assert(b.load_calls==1 && b.save_calls==1);
    assert(!strcmp(b.scene,"boss") && !g.fading && !g.transition_active);
    assert(ck64_transition_tick(&g)==CK64_INVALID_STATE);
}
static void test_fade_fallback(void) {
    FakeBackend b;CK64GameFlow g;setup(&g,&b);
    strcpy(g.last_scene,"tunnel");g.last_door=4;
    g.current_world=-1;g.check_point_door=8;
    assert(ck64_fade_to_scene(&g,"",0)==CK64_OK);
    assert(g.go_to_door==4 && !strcmp(g.pending_scene,"tunnel"));
    assert(ck64_fade_to_scene(&g,"ignored",33)==CK64_OK);
    assert(b.out_calls==1); /* original busy guard */
}
static void test_die_and_return(void) {
    FakeBackend b;CK64GameFlow g;setup(&g,&b);
    g.hp=1;g.check_point_door=-3;g.current_world=1;
    assert(ck64_die(&g)==CK64_OK);
    assert(g.hp==5 && g.check_point_door==0 && g.data.total_deaths==1);
    assert(!strcmp(g.pending_scene,"gameover"));
    setup(&g,&b);g.current_world=-1;
    strcpy(g.last_scene,"field");g.last_door=9;
    assert(ck64_die(&g)==CK64_OK);
    assert(g.hurt_on_enter && g.data.total_deaths==0);
    assert(g.go_to_door==9 && g.next_track==1);
    assert(!strcmp(g.pending_scene,"field"));
}
static void test_failure_paths(void) {
    FakeBackend b;CK64GameFlow g;setup(&g,&b);b.fail_save=true;
    assert(ck64_load_scene(&g,"cavern")==CK64_CALLBACK_FAILED);
    assert(!strcmp(b.scene,"field") && b.load_calls==0);
    b.fail_save=false;b.fail_out=true;
    assert(ck64_fade_to_scene(&g,"boss",0)==CK64_CALLBACK_FAILED);
    assert(!g.fading && !g.transition_active);
}
static void test_update_counter_subset(void) {
    FakeBackend b;CK64GameFlow g;setup(&g,&b);
    g.hp=-1;g.current_world=0;
    assert(ck64_gameflow_update_counters(&g)==CK64_OK);
    assert(g.hp==0 && g.scene_timer==1 && g.active_time==1);
    assert(g.data.game_time==1);
    g.current_world=-100;
    assert(ck64_gameflow_update_counters(&g)==CK64_OK);
    assert(g.scene_timer==2 && g.active_time==2 && g.data.game_time==1);
    g.scene_timer=INT32_MAX;
    assert(ck64_gameflow_update_counters(&g)==CK64_PROGRESS_OUT_OF_BOUNDS);
    assert(g.active_time==2 && g.data.game_time==1);
}
int main(void) {
    test_defaults();test_level_progression();test_volume();test_scene_load();
    test_gameover_lastscene();test_fade_exactly_50_ticks();test_fade_fallback();
    test_die_and_return();test_failure_paths();test_update_counter_subset();
    puts("10 gameflow tests passed");
    return 0;
}
