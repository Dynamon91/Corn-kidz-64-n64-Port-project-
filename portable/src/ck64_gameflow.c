#include "ck64_gameflow.h"
#include <string.h>
#include <limits.h>

static CK64Status copy_scene(char destination[CK64_SCENE_NAME_CAPACITY],
                              const char *source) {
    size_t n;
    if (!source) return CK64_INVALID_ARGUMENT;
    /* Don't read beyond bounded strings from malformed external sources. */
    for (n = 0; n < CK64_SCENE_NAME_CAPACITY; ++n) {
        if (source[n] == '\0') {
            memmove(destination, source, n + 1);
            return CK64_OK;
        }
    }
    return CK64_SCENE_NAME_TOO_LONG;
}

void ck64_gameflow_init(CK64GameFlow *g, CK64Backend backend) {
    if (!g) return;
    memset(g, 0, sizeof(*g));
    g->backend = backend;
    /* ResetSav leaves Data.items etc in zeroed arrays; they are not modeled
       here yet. See reports/gamectrl_phase2.md before implementing save IO. */
    g->data.current_level = 1;
    g->data.max_hp = 8;
    g->start_hp = 5; /* GameCtrl::.ctor */
    g->hp = g->start_hp; /* GameCtrl::Start */
    g->dp = 16;         /* GameCtrl::Start */
    g->level_thresholds[1] = 0;
    g->level_thresholds[2] = 70;
    g->level_thresholds[3] = 140;
    g->level_thresholds[4] = 200;
    g->level_thresholds[5] = 300;
    g->level_thresholds[6] = 360;
    g->level_thresholds[7] = 999;
}

CK64Status ck64_set_current_level(CK64GameFlow *g, bool *increased) {
    int32_t index;
    if (!g) return CK64_INVALID_ARGUMENT;
    if (increased) *increased = false;
    /* Unity would index out of bounds for a corrupt/huge currentLvl.
       Guard explicitly, especially when importing an untrusted save. */
    if (g->data.current_level < 0 ||
        g->data.current_level >= CK64_LEVEL_CAPACITY - 1)
        return CK64_PROGRESS_OUT_OF_BOUNDS;
    index = g->data.current_level + 1;
    if (g->data.item_total >= g->level_thresholds[index]) {
        ++g->data.current_level;
        if (increased) *increased = true;
    }
    return CK64_OK;
}

CK64Status ck64_set_volume(CK64GameFlow *g, int32_t volume,
                            float *listener_gain) {
    if (!g) return CK64_INVALID_ARGUMENT;
    g->data.volume = volume;
    if (listener_gain) *listener_gain = (float)g->data.volume * 0.01f;
    /* Source has no clamp; backend can clamp if required by audio device. */
    return CK64_OK;
}

CK64Status ck64_load_scene(CK64GameFlow *g, const char *name) {
    CK64Status result;
    const char *active;
    const char *target;
    char target_copy[CK64_SCENE_NAME_CAPACITY];
    char active_copy[CK64_SCENE_NAME_CAPACITY];
    if (!g || !name || !g->backend.get_active_scene ||
        !g->backend.load_scene || !g->backend.save_game)
        return CK64_INVALID_ARGUMENT;
    active = g->backend.get_active_scene(g->backend.ctx);
    if (!active) return CK64_INVALID_STATE;
    result = copy_scene(active_copy, active);
    if (result != CK64_OK) return result;
    target = name[0] == '\0' ? active_copy : name;
    result = copy_scene(target_copy, target);
    if (result != CK64_OK) return result;
    /* Original SaveGame runs unconditionally before fading flag is reset.
       When the backend reports failure we abort, rather than silently lose
       persistent data; this is a documented safety divergence. */
    if (!g->backend.save_game(g->backend.ctx)) return CK64_CALLBACK_FAILED;
    g->fading = false;
    if (name[0] != '\0' && strcmp(active_copy, "gameover") != 0)
        (void)copy_scene(g->last_scene, active_copy);
    if (!g->backend.load_scene(g->backend.ctx, target_copy))
        return CK64_CALLBACK_FAILED;
    g->scene_timer = 0;
    return CK64_OK;
}

CK64Status ck64_set_checkpoint_scene(CK64GameFlow *g) {
    const char *active;
    if (!g || !g->backend.get_active_scene) return CK64_INVALID_ARGUMENT;
    active = g->backend.get_active_scene(g->backend.ctx);
    if (!active) return CK64_INVALID_STATE;
    return copy_scene(g->checkpoint_scene, active);
}

static CK64Status begin_transition(CK64GameFlow *g, const char *scene) {
    CK64Status result;
    if (!g->backend.transition_out || !g->backend.load_scene ||
        !g->backend.get_active_scene || !g->backend.save_game)
        return CK64_INVALID_ARGUMENT;
    result = copy_scene(g->pending_scene, scene);
    if (result != CK64_OK) return result;
    if (!g->backend.transition_out(g->backend.ctx))
        return CK64_CALLBACK_FAILED;
    /* Unity StartCoroutine executes MoveNext immediately, which sets
       GameCtrl.bFading and yields once; the coroutine then yields until
       its local counter reaches 50. */
    g->fade_yields = 1;
    g->transition_active = true;
    g->fading = true;
    return CK64_OK;
}

CK64Status ck64_fade_to_scene(CK64GameFlow *g, const char *scene, int32_t door) {
    CK64Status result;
    const char *resolved_scene = scene;
    int32_t resolved_door = door;
    if (!g || !scene) return CK64_INVALID_ARGUMENT;
    if (g->fading) return CK64_OK; /* Exact busy guard from original IL. */
    if (door == 0 && g->check_point_door != 0)
        resolved_door = g->check_point_door;
    if (g->current_world < 0 && g->current_world > -100 && scene[0] == '\0') {
        resolved_door = g->last_door;
        resolved_scene = g->last_scene;
    }
    result = begin_transition(g, resolved_scene);
    if (result != CK64_OK) return result;
    g->go_to_door = resolved_door;
    return CK64_OK;
}

CK64Status ck64_return_to_last_scene(CK64GameFlow *g) {
    CK64Status result;
    if (!g) return CK64_INVALID_ARGUMENT;
    if (g->fading) return CK64_OK;
    if (!g->last_scene[0]) return CK64_INVALID_STATE;
    result = begin_transition(g, g->last_scene);
    if (result != CK64_OK) return result;
    g->go_to_door = g->last_door;
    g->next_track = 1;
    return CK64_OK;
}

CK64Status ck64_die(CK64GameFlow *g) {
    if (!g) return CK64_INVALID_ARGUMENT;
    if (g->check_point_door < 0) g->check_point_door = 0;
    if (g->current_world < 0 && g->last_scene[0] != '\0') {
        g->hurt_on_enter = true;
        return ck64_return_to_last_scene(g);
    }
    g->next_track = 0;
    g->hp = g->start_hp;
    if (g->data.total_deaths == INT32_MAX)
        return CK64_PROGRESS_OUT_OF_BOUNDS;
    ++g->data.total_deaths;
    g->hurt_on_enter = false;
    return ck64_fade_to_scene(g, "gameover", 0);
}

CK64Status ck64_transition_tick(CK64GameFlow *g) {
    CK64Status status;
    if (!g) return CK64_INVALID_ARGUMENT;
    if (!g->transition_active) return CK64_INVALID_STATE;
    if (g->fade_yields < 50u) {
        ++g->fade_yields;
        return CK64_OK;
    }
    /* Source coroutine sets currentWorld to zero immediately before the
       LoadScene call; LoadScene in turn calls SaveGame before scene loading. */
    g->current_world = 0;
    status = ck64_load_scene(g, g->pending_scene);
    if (status != CK64_OK) return status;
    /* Following scene load the original coroutine yields once more; scene
       lifetime is backend-owned and this portable core is now idle. */
    g->transition_active = false;
    g->fade_yields = 0;
    g->pending_scene[0] = '\0';
    return CK64_OK;
}

CK64Status ck64_gameflow_update_counters(CK64GameFlow *g) {
    if (!g) return CK64_INVALID_ARGUMENT;
    if (g->scene_timer == INT32_MAX || g->active_time == INT32_MAX ||
        (g->current_world > -100 && g->data.game_time == INT32_MAX))
        return CK64_PROGRESS_OUT_OF_BOUNDS;
    ++g->scene_timer;
    ++g->active_time;
    if (g->current_world > -100) ++g->data.game_time;
    if (g->hp < 0) g->hp = 0;
    /* MusicFade, debug mode and input processing deliberately not ported. */
    return CK64_OK;
}
