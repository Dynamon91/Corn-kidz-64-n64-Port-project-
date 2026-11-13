/* Corn Kidz 64 -> N64: Phase 12 DIAGNOSTIC frontend.
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
#include "wireframe.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
    unsigned grid_mode; /* FULL, LOW or OFF: controller R */
    unsigned previous_draw_us; /* CPU time for previous draw; not emulator VPS */
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
        return;
    }
    pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    if (pressed.z) {
        app.view_3d = !app.view_3d;
        app.jump_pending = false;
        app.reset_pending = false;
    }
    if (app.view_3d) {
        if (pressed.r) {
            app.grid_mode = app.grid_mode == CK64_GRID_FULL ? CK64_GRID_SPARSE
                          : app.grid_mode == CK64_GRID_SPARSE ? CK64_GRID_OFF
                          : CK64_GRID_FULL;
        }
        if (pressed.start)
            (void)ck64_world3d_set_collision_enabled(
                &app.world, !app.world.collision_enabled);
        joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
        app.stick_x = (float)inputs.stick_x / 85.0f;
        app.stick_y = (float)inputs.stick_y / 85.0f;
        app.camera_turn = (inputs.btn.c_right ? 1.0f : 0.0f) -
                          (inputs.btn.c_left ? 1.0f : 0.0f);
        app.jump_pending |= pressed.a;
        app.reset_pending |= pressed.b;
        return;
    }
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
    graphics_draw_text(screen, 15, 40, "Phase 12: grid audit / 30 Hz");
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
    graphics_draw_text(screen, 15, 184, "A: FADE TEST | B: RETURN TEST");
    graphics_draw_text(screen, 15, 202, "START: MUSIC  Z: 3D VIEW");
    if (app.last_runtime_error != CK64_RUNTIME_OK ||
        app.last_control_error != CK64_OK || app.dropped_update) {
        snprintf(line, sizeof line, "ERR runtime=%d ctrl=%d drop=%d",
                (int)app.last_runtime_error,
                (int)app.last_control_error, app.dropped_update ? 1 : 0);
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
                    if (!ck64_world3d_step(&app.world, input)) {
                        app.dropped_update = true;
                        break;
                    }
                    input.jump = false; input.reset = false;
                    app.jump_pending = false;
                    app.reset_pending = false;
                }
            }
        }
        if (app.view_3d) {
            surface_t *screen=display_get();
            if (screen) {
                /* Measure CPU time only while drawing pixels/text/lines.
                 * Excludes display_get and display_show waits: do not
                 * interpret this as emulator VPS or GPU frame time. */
                const uint64_t draw_begin = (uint64_t)timer_ticks();
                ck64_wireframe_draw(screen,&app.world,
                        (unsigned long)app.runtime.fixed_updates,
                        app.grid_mode,app.previous_draw_us,&app.line_stats);
                const uint64_t draw_end = (uint64_t)timer_ticks();
                if (draw_end >= draw_begin) {
                    const uint64_t elapsed = draw_end - draw_begin;
                    /* Avoid overflow on pathological timers/hangs. */
                    const uint64_t cap_ticks = ((uint64_t)TICKS_PER_SECOND * 99999u)
                                              / 1000000u;
                    app.previous_draw_us = elapsed > cap_ticks ? 99999u
                        : (unsigned)((elapsed * 1000000u) /
                                     (uint64_t)TICKS_PER_SECOND);
                }
                display_show(screen);
            }
        } else draw_diagnostics();
    }
}
