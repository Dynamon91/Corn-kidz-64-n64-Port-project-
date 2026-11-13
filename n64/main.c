/* Corn Kidz 64 -> N64: Phase 19 SOURCE-DRIVEN DIAGNOSTIC frontend.
 * Includes fixed-step GameCtrl/MusicFade plus a perspective wireframe TEST.
 * The navigation values are experimental, NOT extracted PlayerCtrl logic.
 * It is NOT a port of original levels, models, textures, music or physics.
 * No save is written to EEPROM: all saves are RAM-only test records.
 */
#include <libdragon.h>
#include "ck64_runtime.h"
#include "ck64_save.h"
#include "ck64_gameflow_save.h"
#include "ck64_world3d.h"
#include "ck64_player_state0.h"
#include "ck64_player_footer.h"
#include "ck64_source_lab.h"
#include "wireframe.h"
#include "scene_anchor_meta.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Phase 20: exact Unity BuildSettings scene order, metadata only.
 * Original level meshes and scripts are NOT loaded by this diagnostic. */
typedef struct CK64UnitySceneInfo {
    const char *name;
    unsigned game_objects;
    unsigned mesh_filters;
} CK64UnitySceneInfo;
static const CK64UnitySceneInfo original_scenes[] = {
    {"launcher", 69u, 2u},
    {"title01", 322u, 49u},
    {"PizzaOut", 618u, 105u},
    {"TutorialWorld01", 1597u, 477u},
    {"Wolloh's Hollow", 8553u, 1376u},
    {"Tower00", 2685u, 395u},
    {"bonus01", 379u, 76u},
    {"bonus02", 336u, 54u},
    {"gameover", 326u, 51u},
    {"credits", 1312u, 54u},
    {"bonus03", 364u, 90u},
    {"bonus04", 374u, 77u},
    {"bonus05", 342u, 61u},
    {"TowerN00", 2341u, 442u},
    {"TestZoneX", 1004u, 328u},
    {"secretZone00", 660u, 196u},
    {"results", 337u, 49u},
    {"bonus06", 392u, 76u},
};
#define CK64_SCENE_COUNT ((unsigned)(sizeof(original_scenes) / sizeof(original_scenes[0])))

/* Keep static; don't use an 8 KB automatic stack frame on R4300i. */
typedef struct CK64DiagnosticApp {
    CK64Runtime runtime;
    CK64SaveData data;
    uint8_t ram_save[CK64_SAVE_RECORD_SIZE];
    char active_scene[CK64_SCENE_NAME_CAPACITY];
    unsigned saved_to_ram;
    unsigned loaded_diagnostic_scene;
    bool ram_save_valid;
    bool fade_started;
    CK64RuntimeStatus last_runtime_error;
    CK64Status last_control_error;
    bool dropped_update;
    unsigned boot_stage;
    CK64World3D world;
    bool view_3d;
    bool jump_pending, reset_pending;
    float stick_x, stick_y, camera_turn;
    bool source_jump_probe;
    bool source_live;
    bool source_a_held, source_release_pending;
    CK64SourceLab source_lab;
    bool source_jump_valid;
    unsigned source_jump_samples;
    CK64PlayerRunFooter source_floor;
    bool source_floor_valid;
    unsigned source_landings;
    CK64State0 source_jump_last;
    CK64State0Result source_jump_result;
    unsigned grid_mode; /* FULL, LOW or OFF: controller R */
    unsigned original_scene_index; /* Z diagnostics: R cycles metadata */
    CK64PerfTimes previous_perf; /* Previous complete display frame, not VPS */
    CK64WireframeStats line_stats;
} CK64DiagnosticApp;
static CK64DiagnosticApp app;

static const char *get_active_scene(void *context) {
    return ((CK64DiagnosticApp *)context)->active_scene;
}

/* This is a volatile scratch snapshot. It is NOT a cartridge EEPROM write. */
static bool save_game_ram(void *context) {
    CK64DiagnosticApp *a = context;
    size_t n = 0;
    if (ck64_gameflow_export_data(&a->runtime.flow, &a->data) != CK64_OK)
        return false;
    if (ck64_save_encode(&a->data, a->ram_save, sizeof(a->ram_save), &n)
        != CK64_SAVE_OK || n != sizeof(a->ram_save))
        return false;
    a->ram_save_valid = true;
    ++a->saved_to_ram;
    return true;
}

/* Deliberately does NOT load original Unity scene data. */
static bool load_diagnostic_scene(void *context, const char *name) {
    CK64DiagnosticApp *a = context;
    const size_t length = strlen(name);
    if (length >= sizeof a->active_scene) return false;
    memcpy(a->active_scene, name, length + 1);
    ++a->loaded_diagnostic_scene;
    a->fade_started = false;
    return true;
}

static bool transition_diagnostic_out(void *context) {
    ((CK64DiagnosticApp *)context)->fade_started = true;
    return true;
}

static void init_diagnostic(void) {
    CK64Backend backend = {
        &app, get_active_scene, save_game_ram,
        load_diagnostic_scene, transition_diagnostic_out
    };
    memcpy(app.active_scene, "diagnostic", sizeof "diagnostic");
    ck64_save_init_fresh(&app.data);
    app.data.bMusic = true; /* Explicit demonstration state, not Unity defaults. */
    if (ck64_runtime_init(&app.runtime, backend,
                          (uint64_t)TICKS_PER_SECOND) != CK64_RUNTIME_OK) {
        app.last_runtime_error = CK64_RUNTIME_BAD_ARGUMENT;
        return;
    }
    (void)ck64_gameflow_import_data(&app.runtime.flow, &app.data);
    app.runtime.music.music_enabled = app.data.bMusic;
    ck64_world3d_reset(&app.world);
    (void)ck64_source_lab_init(&app.source_lab);
    app.view_3d = true;
    app.grid_mode = CK64_GRID_FULL;
}

static void controller_diagnostics(void) {
    joypad_buttons_t pressed;
    if (!joypad_is_connected(JOYPAD_PORT_1)) {
        app.stick_x=0.0f;
        app.stick_y=0.0f;
        app.camera_turn=0.0f;
        app.jump_pending=false;
        app.reset_pending=false;
        app.source_a_held=false;
        app.source_release_pending=false;
        return;
    }
    pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if (pressed.z) {
        app.view_3d = !app.view_3d;
        app.jump_pending = false;
        app.reset_pending = false;
    }
    if (app.view_3d) {
        if (pressed.l) {
            /* Preserve legacy diagnostic: L cycles LEGACY -> PROBE -> LIVE. */
            if (app.source_live) {
                app.source_live=false;
                app.source_jump_probe=false;
            } else if (app.source_jump_probe) {
                app.source_jump_probe=false;
                app.source_live=true;
                (void)ck64_source_lab_init(&app.source_lab);
            } else {
                app.source_jump_probe=true;
                app.source_live=false;
                app.source_jump_valid=false;
                app.source_jump_samples=0;
                app.source_floor=(CK64PlayerRunFooter){0};
                app.source_floor.on_floor=app.world.grounded;
                app.source_floor.on_floor_timer=app.world.grounded ? 1 : -1;
                app.source_floor_valid=true;
                app.source_landings=0;
            }
            app.jump_pending=false;
            app.reset_pending=false;
            app.source_a_held=false;
            app.source_release_pending=false;
        }
        if (app.source_jump_probe && pressed.a) {
            /* One-shot source rule sample, NOT modifying world3d physics. */
            CK64State0 sample={0};
            CK64State0Result result={0};
            sample.on_floor=app.world.grounded;
            sample.on_floor_timer=app.source_floor.on_floor_timer;
            sample.jump_num=app.source_jump_valid ? app.source_jump_last.jump_num : 0;
            sample.z_mov=0.4f;
            sample.y_vel=app.world.velocity.y;
            sample.forward.z=1.0f;
            sample.last_ctrl_dir.z=1.0f;
            app.source_jump_valid=ck64_player_state0_step(
                &sample,(CK64State0Buttons){.a_down=true},&result);
            if (app.source_jump_valid) {
                app.source_jump_last=sample;
                app.source_jump_result=result;
                ++app.source_jump_samples;
            }
        }
        if (pressed.r) {
            app.grid_mode = app.grid_mode == CK64_GRID_FULL ? CK64_GRID_SPARSE
                          : app.grid_mode == CK64_GRID_SPARSE ? CK64_GRID_OFF
                          : CK64_GRID_FULL;
        }
        if (pressed.start) {
            CK64World3D *w = app.source_live ? &app.source_lab.world : &app.world;
            (void)ck64_world3d_set_collision_enabled(w, !w->collision_enabled);
        }
        joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
        app.stick_x = (float)inputs.stick_x / 85.0f;
        app.stick_y = (float)inputs.stick_y / 85.0f;
        app.camera_turn = (inputs.btn.c_right ? 1.0f : 0.0f) -
                          (inputs.btn.c_left ? 1.0f : 0.0f);
        if (app.source_live) {
            app.source_release_pending |= app.source_a_held && !inputs.btn.a;
            app.source_a_held=inputs.btn.a;
        }
        app.jump_pending |= pressed.a;
        app.reset_pending |= pressed.b;
        return;
    }
    if (pressed.r)
        app.original_scene_index = (app.original_scene_index + 1u) % CK64_SCENE_COUNT;
    if (pressed.a) {
        const char *destination = strcmp(app.active_scene, "diagnostic") == 0
                                  ? "test_room" : "diagnostic";
        app.last_control_error = ck64_fade_to_scene(&app.runtime.flow,
                                                     destination, 1);
    }
    if (pressed.b && app.runtime.flow.last_scene[0] != '\0') {
        app.last_control_error = ck64_return_to_last_scene(&app.runtime.flow);
    }
    if (pressed.start) {
        app.data.bMusic = !app.data.bMusic;
        app.runtime.music.music_enabled = app.data.bMusic;
    }
}

/* Restore the three stable boot markers seen on ares in Phase 6. */
static void draw_boot_stage(const char *message) {
    surface_t *screen = display_get();
    if (!screen) return;
    graphics_fill_screen(screen, graphics_make_color(13,19,29,255));
    graphics_draw_text(screen,12,36,"CK64 - BOOT DIAGNOSTIC");
    graphics_draw_text(screen,12,70,message);
    graphics_draw_text(screen,12,118,"NOT THE FULL CORN KIDZ 64 GAME");
    display_show(screen);
}

static void draw_diagnostics(void) {
    char line[120];
    surface_t *screen = display_get();
    if (!screen) return;
    graphics_fill_screen(screen, graphics_make_color(13, 19, 29, 255));
    graphics_draw_text(screen, 15, 18, "CORN KIDZ 64 - N64 ENGINE DIAGNOSTIC");
    graphics_draw_text(screen, 15, 40, "PH21: ORIGINAL WORLD ANCHORS");
    graphics_draw_text(screen, 15, 60, "NO ORIGINAL GAME LEVELS OR ASSETS");
    snprintf(line, sizeof line, "Scene: %.90s", app.active_scene);
    graphics_draw_text(screen, 15, 82, line);
    snprintf(line, sizeof line, "Fixed updates: %lu", (unsigned long)app.runtime.fixed_updates);
    graphics_draw_text(screen, 15, 100, line);
    snprintf(line, sizeof line, "Fade: %u / 50", app.runtime.flow.fade_yields);
    graphics_draw_text(screen, 15, 118, line);
    snprintf(line, sizeof line, "RAM saves: %u (EEPROM NOT ACTIVE)", app.saved_to_ram);
    graphics_draw_text(screen, 15, 137, line);
    snprintf(line, sizeof line, "Music flag: %s (AUDIO NOT ACTIVE)",
             app.data.bMusic ? "ON" : "OFF");
    graphics_draw_text(screen, 15, 156, line);
    const CK64UnitySceneInfo *scene = &original_scenes[app.original_scene_index];
    snprintf(line, sizeof line, "SOURCE %u/%u: %.23s",
             app.original_scene_index + 1u, CK64_SCENE_COUNT, scene->name);
    graphics_draw_text(screen, 15, 177, line);
    snprintf(line, sizeof line, "GO:%u MESH:%u (META ONLY)",
             scene->game_objects, scene->mesh_filters);
    graphics_draw_text(screen, 15, 191, line);
    if (app.original_scene_index == 3u) {
        /* These are source world-space metadata, NOT drawn Unity meshes. */
        graphics_draw_text(screen, 15, 205, CK64_TUTORIAL_PLAYER_WORLD);
        graphics_draw_text(screen, 15, 219, CK64_TUTORIAL_SPAWN_WORLD);
    } else {
        graphics_draw_text(screen, 15, 205, "R:SCENE A:FADE B:BACK Z:3D");
    }
    if (app.last_runtime_error != CK64_RUNTIME_OK ||
        app.last_control_error != CK64_OK || app.dropped_update) {
        snprintf(line, sizeof line, "ERR runtime=%d ctrl=%d drop=%d",
                (int)app.last_runtime_error,
                (int)app.last_control_error, app.dropped_update ? 1 : 0);
        graphics_draw_box(screen, 10, 216, 303, 19,
                          graphics_make_color(75,25,30,255));
        graphics_draw_text(screen, 15, 221, line);
    }
    /* Visible fade progress (not Unity's original transition graphic). */
    if (app.fade_started) {
        const unsigned width = (app.runtime.flow.fade_yields * 180u) / 50u;
        graphics_draw_box(screen, 125, 175, (int)width, 4,
                          graphics_make_color(250, 183, 81, 255));
    }
    display_show(screen);
}

int main(void) {
    CK64ClockResult frame = {0};
    /* 16-bit 320x240x2 framebuffers = 300 KiB; no 3D resources loaded. */
    timer_init();
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2,
                 GAMMA_NONE, FILTERS_RESAMPLE);
    graphics_set_default_font();
    app.boot_stage=1;
    draw_boot_stage("BOOT 1: DISPLAY INITIALIZED");
    joypad_init();
    app.boot_stage=2;
    draw_boot_stage("BOOT 2: CONTROLLER INITIALIZED");
    init_diagnostic();
    app.boot_stage=3;
    draw_boot_stage("BOOT 3: GAMEFLOW INITIALIZED");
    for (;;) {
        joypad_poll();
        controller_diagnostics();
        if (app.last_runtime_error == CK64_RUNTIME_OK) {
            app.last_runtime_error = ck64_runtime_frame(&app.runtime,
                        (uint64_t)timer_ticks(), 1u, &frame);
            app.dropped_update |= frame.dropped_time;
            if (app.view_3d) {
                CK64WorldInput input = {
                    app.stick_x, app.stick_y, app.camera_turn,
                    app.jump_pending, app.reset_pending
                };
                for (uint32_t i=0; i<frame.updates; ++i) {
                    if (app.source_live) {
                        CK64SourceLabInput s_in={
                            input.stick_x,input.stick_y,input.camera_turn,
                            input.jump,app.source_release_pending,input.reset
                        };
                        if (!ck64_source_lab_step(&app.source_lab,s_in)) {
                            app.dropped_update=true;
                            break;
                        }
                    } else if (!ck64_world3d_step(&app.world, input)) {
                        app.dropped_update = true;
                        break;
                    }
                    if (app.source_jump_probe && app.source_floor_valid) {
                        if (input.reset) {
                            app.source_floor=(CK64PlayerRunFooter){0};
                            app.source_landings=0;
                            app.source_jump_valid=false;
                        }
                        app.source_floor.on_floor=app.world.grounded;
                        app.source_floor.pl_state=0;
                        CK64PlayerRunFooterResult source_footer_event={0};
                        app.source_floor_valid=ck64_player_run_footer_step(
                            &app.source_floor,&source_footer_event);
                        if (app.source_floor_valid &&
                            source_footer_event.landing_edge &&
                            app.source_landings != UINT32_MAX)
                            ++app.source_landings;
                        if (!app.source_floor_valid)
                            app.dropped_update=true;
                    }
                    input.jump = false; input.reset = false;
                    app.jump_pending = false;
                    app.reset_pending = false;
                    app.source_release_pending=false;
                }
            }
        }
        if (app.view_3d) {
            const uint64_t before_get=(uint64_t)timer_ticks();
            surface_t *screen=display_get();
            const uint64_t after_get=(uint64_t)timer_ticks();
            if (screen) {
                ck64_wireframe_draw(screen,
                        app.source_live ? &app.source_lab.world : &app.world,
                        (unsigned long)app.runtime.fixed_updates,
                        app.grid_mode,&app.previous_perf,&app.line_stats);
                if (app.source_live) {
                    char lab_line[100];
                    graphics_draw_box(screen,0,150,320,61,
                        graphics_make_color(19,31,44,255));
                    graphics_draw_text(screen,5,153,
                        "PH19 SOURCE LIVE - L:MODE");
                    snprintf(lab_line,sizeof lab_line,
                        "zMov:%.2f  yVel:%.2f  floorT:%ld",
                        (double)app.source_lab.motion.z_mov,
                        (double)app.source_lab.state0.y_vel,
                        (long)app.source_lab.floor_history.on_floor_timer);
                    graphics_draw_text(screen,5,168,lab_line);
                    snprintf(lab_line,sizeof lab_line,
                        "J:%u LAND:%u PATH:%u",
                        app.source_lab.jumps,app.source_lab.landings,
                        (unsigned)app.source_lab.last_decision.path);
                    graphics_draw_text(screen,5,183,lab_line);
                    graphics_draw_text(screen,5,198,
                        "TEST GEOMETRY / NO ORIGINAL ASSETS");
                } else if (app.source_jump_probe) {
                    char probe_line[96];
                    graphics_draw_box(screen,3,185,313,52,
                        graphics_make_color(18,26,40,255));
                    graphics_draw_text(screen,7,189,
                        "L SOURCE FLOOR/JUMP PROBE");
                    snprintf(probe_line,sizeof probe_line,
                        "floorT=%ld land=%u step=%ld",
                        (long)app.source_floor.on_floor_timer,
                        app.source_landings,
                        (long)app.source_floor.pl_timer);
                    graphics_draw_text(screen,7,204,probe_line);
                    if (app.source_jump_valid) {
                        snprintf(probe_line,sizeof probe_line,
                            "A y=%.2f jump=%ld flags=%lu",
                            (double)app.source_jump_last.y_vel,
                            (long)app.source_jump_last.jump_num,
                            (unsigned long)app.source_jump_result.effects);
                        graphics_draw_text(screen,7,219,probe_line);
                    } else graphics_draw_text(screen,7,219,
                        "Press A to sample source jump");
                }
                const uint64_t after_draw=(uint64_t)timer_ticks();
                display_show(screen);
                const uint64_t after_show=(uint64_t)timer_ticks();
                CK64PerfTimes next={0};
                if (ck64_perf_measure(&next,before_get,after_get,after_draw,
                                      after_show,(uint64_t)TICKS_PER_SECOND))
                    app.previous_perf=next;
                else app.previous_perf.valid=false;
            } else app.previous_perf.valid=false;
        } else draw_diagnostics();
    }
}
