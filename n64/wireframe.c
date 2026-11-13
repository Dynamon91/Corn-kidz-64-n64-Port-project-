/* CPU wireframe audit: draw grid in three densities for ares comparison.
 * No original Corn Kidz 64 models, textures or gameplay are present.
 */
#include "wireframe.h"
#include <stdio.h>

static void line3d(surface_t *screen, const CK64World3D *w,
                   CK64Vec3 from, CK64Vec3 to, uint32_t color,
                   CK64WireframeStats *stats) {
    CK64ScreenPoint a, b;
    ++stats->lines_attempted;
    if (!ck64_world3d_project_line(w, from, to, &a, &b)) return;
    graphics_draw_line(screen, (int)a.x, (int)a.y, (int)b.x, (int)b.y, color);
    ++stats->lines_drawn;
}

static void box3d(surface_t *screen, const CK64World3D *w,
                  float x,float y,float z,float half_width,float height,
                  uint32_t color, CK64WireframeStats *stats) {
    const CK64Vec3 v[8] = {
        {x-half_width,y,z-half_width}, {x+half_width,y,z-half_width},
        {x+half_width,y,z+half_width}, {x-half_width,y,z+half_width},
        {x-half_width,y+height,z-half_width}, {x+half_width,y+height,z-half_width},
        {x+half_width,y+height,z+half_width}, {x-half_width,y+height,z+half_width}
    };
    static const unsigned char edges[][2] = {
        {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };
    for (unsigned i=0; i<12; ++i)
        line3d(screen, w, v[edges[i][0]], v[edges[i][1]], color, stats);
}

void ck64_wireframe_draw(surface_t *screen, const CK64World3D *world,
                         unsigned long game_updates,
                         unsigned grid_mode, unsigned previous_draw_us,
                         CK64WireframeStats *stats) {
    CK64WireframeStats ignored = {0};
    if (!screen || !world) return;
    if (!stats) stats = &ignored;
    *stats = (CK64WireframeStats){0, 0};
    if (grid_mode > CK64_GRID_FULL) grid_mode = CK64_GRID_FULL;

    const uint32_t grid = graphics_make_color(57,96,90,255);
    const uint32_t border = graphics_make_color(101,190,130,255);
    const uint32_t avatar = graphics_make_color(255,208,111,255);
    const uint32_t landmark =
        (world->collided_this_step || world->contact_flash_steps > 0 ||
         world->ejection_flash_steps > 0)
          ? graphics_make_color(255,104,66,255)
          : graphics_make_color(132,177,239,255);
    graphics_fill_screen(screen, graphics_make_color(13,21,29,255));

    /* 18 grid lines FULL, 10 SPARSE, 0 OFF. Four boundary lines
     * and 24 object-box edges are always drawn for test consistency.
     */
    if (grid_mode != CK64_GRID_OFF) {
        const int stride = (grid_mode == CK64_GRID_FULL) ? 2 : 4;
        for (int i=-8; i<=8; i+=stride) {
            line3d(screen,world,(CK64Vec3){(float)i,0.0f,-8.0f},
                   (CK64Vec3){(float)i,0.0f,8.0f},grid,stats);
            line3d(screen,world,(CK64Vec3){-8.0f,0.0f,(float)i},
                   (CK64Vec3){8.0f,0.0f,(float)i},grid,stats);
        }
    }

    line3d(screen,world,(CK64Vec3){-8,0,-8},(CK64Vec3){8,0,-8},border,stats);
    line3d(screen,world,(CK64Vec3){8,0,-8},(CK64Vec3){8,0,8},border,stats);
    line3d(screen,world,(CK64Vec3){8,0,8},(CK64Vec3){-8,0,8},border,stats);
    line3d(screen,world,(CK64Vec3){-8,0,8},(CK64Vec3){-8,0,-8},border,stats);
    box3d(screen,world,CK64_TEST_PILLAR_X,0,CK64_TEST_PILLAR_Z,
          CK64_TEST_PILLAR_HALF,CK64_TEST_PILLAR_HEIGHT,landmark,stats);
    box3d(screen,world,world->position.x,world->position.y,world->position.z,
          CK64_TEST_AVATAR_HALF,CK64_TEST_AVATAR_HEIGHT,avatar,stats);

    /* Contact feedback unchanged from successful ares Phase 11 test. */
    if (world->ejection_flash_steps > 0) {
        graphics_draw_box(screen,105,52,110,15,
                          graphics_make_color(101,43,34,255));
        graphics_draw_text(screen,123,55,"EJECTED");
    } else if (world->collided_this_step || world->contact_flash_steps > 0) {
        graphics_draw_box(screen,105,52,110,15,
                          graphics_make_color(101,43,34,255));
        graphics_draw_text(screen,123,55,"CONTACT!");
    }

    graphics_draw_box(screen,0,0,320,32,graphics_make_color(8,15,22,255));
    graphics_draw_text(screen,6,4,"CK64 N64 - PHASE 12 FRAME AUDIT");
    graphics_draw_text(screen,6,18,"STICK MOVE A:JUMP B:RESET R:GRID");

    /* A short diagnostic bar shows PREVIOUS draw CPU microseconds (not VPS).
     * Drawn/attempted line counts refer to this frame, after near-plane
     * and screen clipping. display_show time is deliberately excluded.
     */
    const char *mode = grid_mode==CK64_GRID_OFF ? "OFF" :
                      grid_mode==CK64_GRID_SPARSE ? "LOW" : "FULL";
    char debug[68];
    graphics_draw_box(screen,0,33,320,14,graphics_make_color(8,15,22,255));
    snprintf(debug,sizeof(debug),"GRID:%s L:%u/%u CPU:%uus",
             mode,stats->lines_drawn,stats->lines_attempted,previous_draw_us);
    graphics_draw_text(screen,6,36,debug);

    graphics_draw_box(screen,0,211,320,29,graphics_make_color(8,15,22,255));
    graphics_draw_text(screen,6,215,world->collision_enabled
        ? "C-L/R CAM START COLL:ON  Z:DIAG"
        : "C-L/R CAM START COLL:OFF Z:DIAG");
    char status[75];
    snprintf(status,sizeof(status),"X:%+.1f Z:%+.1f Y:%.1f H:%u E:%u",
             world->position.x,world->position.z,world->position.y,
             world->collision_hits,world->contact_events);
    graphics_draw_text(screen,6,228,status);
    (void)game_updates; /* Game step counter remains in Z diagnostic view. */
}
