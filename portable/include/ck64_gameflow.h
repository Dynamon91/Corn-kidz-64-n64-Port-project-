#ifndef CK64_GAMEFLOW_H
#define CK64_GAMEFLOW_H
/* Corn Kidz 64 -> N64: scene/progression extraction, platform-neutral C11.
 * Not a full GameCtrl port. Every behavior implemented here is tied to an
 * original managed method, with conservative safety checks documented.
 * No dependency on Unity, stdio, heap allocation, libdragon or Windows.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { CK64_SCENE_NAME_CAPACITY = 128, CK64_LEVEL_CAPACITY = 99 };

typedef enum CK64Status {
    CK64_OK = 0,
    CK64_INVALID_ARGUMENT,
    CK64_INVALID_STATE,
    CK64_SCENE_NAME_TOO_LONG,
    CK64_CALLBACK_FAILED,
    CK64_PROGRESS_OUT_OF_BOUNDS
} CK64Status;

/* Backend functions have no N64 dependency. The concrete target implementation
 * can later provide ROM scene switching, audio transitions and save-media IO.
 * A true return means the operation succeeded (apart from get_active_scene).
 * Name returned by get_active_scene must remain valid until next backend call.
 */
typedef struct CK64Backend {
    void *ctx;
    const char *(*get_active_scene)(void *ctx);
    bool (*save_game)(void *ctx);
    bool (*load_scene)(void *ctx, const char *name);
    bool (*transition_out)(void *ctx);
} CK64Backend;

/* Only the Data fields used by these methods, not a complete Data struct.
 * Original managed save uses .NET BinaryFormatter; NOT binary-compatible.
 */
typedef struct CK64DataCore {
    int32_t item_total;
    int32_t current_level;
    int32_t total_deaths;
    int32_t max_hp;
    int32_t volume;
    int32_t game_time;
} CK64DataCore;

typedef struct CK64GameFlow {
    CK64Backend backend;
    CK64DataCore data;
    int32_t level_thresholds[CK64_LEVEL_CAPACITY];
    int32_t start_hp;
    int32_t hp;
    int32_t dp;
    int32_t current_world;
    int32_t check_point_door;
    int32_t last_door;
    int32_t go_to_door;
    int32_t next_track;
    int32_t scene_timer;
    int32_t active_time;
    bool hurt_on_enter;
    bool fading;
    bool transition_active;
    unsigned fade_yields; /* Coroutine starts immediately and yields once. */
    char last_scene[CK64_SCENE_NAME_CAPACITY];
    char checkpoint_scene[CK64_SCENE_NAME_CAPACITY];
    char pending_scene[CK64_SCENE_NAME_CAPACITY];
} CK64GameFlow;

/* GameCtrl.ResetSav, GameCtrl.Awake threshold table, GameCtrl.Start and ctor. */
void ck64_gameflow_init(CK64GameFlow *g, CK64Backend backend);

/* GameCtrl.SetCurrentLvl: increments AT MOST once per call, no auto-loop. */
CK64Status ck64_set_current_level(CK64GameFlow *g, bool *increased);

/* GameCtrl.SetVolume stores un-clamped input; 0.01f scaling for AudioListener. */
CK64Status ck64_set_volume(CK64GameFlow *g, int32_t volume, float *listener_gain);

/* GameCtrl.LoadScene: calls SaveGame, handles empty-name reload, remembers last
 * scene unless active scene is 'gameover', and resets scene timer/fade flag.
 */
CK64Status ck64_load_scene(CK64GameFlow *g, const char *name);

/* GameCtrl.SetCheckpointScene: uses current active scene. */
CK64Status ck64_set_checkpoint_scene(CK64GameFlow *g);

/* GameCtrl.FadeToScene / Return / Die. The transition requires a host tick to
 * resume the coroutine after its initial synchronous yield. 50 subsequent
 * successful ticks are required before scene loading is attempted.
 */
CK64Status ck64_fade_to_scene(CK64GameFlow *g, const char *scene, int32_t door);
CK64Status ck64_return_to_last_scene(CK64GameFlow *g);
CK64Status ck64_die(CK64GameFlow *g);
CK64Status ck64_transition_tick(CK64GameFlow *g);

/* Only the counter/HP subset at the start of GameCtrl.Update: per-frame
 * sceneTimer and activeTime increments; gameTime increments only when
 * currentWorld > -100; negative HP clamps to zero. MusicFade/debug inputs
 * are NOT implemented. Guard overflows (deliberate divergence from IL add).
 */
CK64Status ck64_gameflow_update_counters(CK64GameFlow *g);

#ifdef __cplusplus
}
#endif
#endif
