/* Corn Kidz 64 -> N64: Phase 5 DIAGNOSTIC frontend.
 * This shows fixed-step GameCtrl/MusicFade and controller callbacks only.
 * It is NOT a port of original levels, models, textures, music or physics.
 * No save is written to EEPROM: all saves are RAM-only test records.
 */
#include <libdragon.h>
#include "ck64_runtime.h"
#include "ck64_save.h"
#include "ck64_gameflow_save.h"
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
    unsigned boot_stage; /* Set only after the corresponding stage succeeds. */
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
}

static void controller_diagnostics(void) {
    joypad_buttons_t pressed;
    if (!joypad_is_connected(JOYPAD_PORT_1)) return;
    pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
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

/* Display boot-stage markers BEFORE controller and game initialization.
 * If the ROM never displays "BOOT 1", failure precedes the C frontend.
 * It may be an IPL3/emulator mismatch, not GameCtrl logic.
 */
static void draw_boot_stage(const char *message) {
    surface_t *screen = display_get();
    if (!screen) return;
    graphics_fill_screen(screen, graphics_make_color(13, 19, 29, 255));
    graphics_draw_text(screen, 12, 36, "CK64 - BOOT DIAGNOSTIC");
    graphics_draw_text(screen, 12, 70, message);
    graphics_draw_text(screen, 12, 118, "NOT THE FULL CORN KIDZ 64 GAME");
    display_show(screen);
}

static void draw_diagnostics(void) {
    char line[120];
    surface_t *screen = display_get();
    if (!screen) return;
    graphics_fill_screen(screen, graphics_make_color(13, 19, 29, 255));
    graphics_draw_text(screen, 15, 18, "CORN KIDZ 64 - N64 ENGINE DIAGNOSTIC");
    graphics_draw_text(screen, 15, 40, "Phase 6: boot staging / 30 Hz loop");
    graphics_draw_text(screen, 15, 60, "NO ORIGINAL GAME LEVELS OR ASSETS");
    snprintf(line, sizeof line, "Boot: %u | Scene: %.70s", app.boot_stage, app.active_scene);
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
    graphics_draw_text(screen, 15, 202, "START: MUSIC FLAG (SILENT)");
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
    app.boot_stage = 1;
    draw_boot_stage("BOOT 1: DISPLAY INITIALIZED");
    joypad_init();
    app.boot_stage = 2;
    draw_boot_stage("BOOT 2: CONTROLLER INITIALIZED");
    init_diagnostic();
    app.boot_stage = 3;
    draw_boot_stage("BOOT 3: GAMEFLOW INITIALIZED");
    for (;;) {
        joypad_poll();
        controller_diagnostics();
        if (app.last_runtime_error == CK64_RUNTIME_OK) {
            app.last_runtime_error = ck64_runtime_frame(&app.runtime,
                        (uint64_t)timer_ticks(), 1u, &frame);
            app.dropped_update |= frame.dropped_time;
        }
        draw_diagnostics();
    }
}
