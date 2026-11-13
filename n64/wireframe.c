#include "wireframe.h"
#include <stdio.h>

static void line3d(surface_t *screen,const CK64World3D *w,
                   CK64Vec3 from, CK64Vec3 to, uint32_t color) {
    CK64ScreenPoint a,b;
    if (!ck64_world3d_project_line(w,from,to,&a,&b)) return;
    graphics_draw_line(screen,(int)a.x,(int)a.y,(int)b.x,(int)b.y,color);
}
/* 12 edges of a simple upright bounding box: placeholder capsule proxy. */
static void box3d(surface_t *screen,const CK64World3D *w,
                  float x,float y,float z,float half_width,float height,
                  uint32_t color) {
    const CK64Vec3 c[8] = {
      {x-half_width,y,z-half_width}, {x+half_width,y,z-half_width},
      {x+half_width,y,z+half_width}, {x-half_width,y,z+half_width},
      {x-half_width,y+height,z-half_width}, {x+half_width,y+height,z-half_width},
      {x+half_width,y+height,z+half_width}, {x-half_width,y+height,z+half_width}
    };
    static const unsigned char edges[][2]={{0,1},{1,2},{2,3},{3,0},
       {4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
    for(unsigned i=0;i<12;i++) line3d(screen,w,c[edges[i][0]],c[edges[i][1]],color);
}
void ck64_wireframe_draw(surface_t *screen, const CK64World3D *world,
                          unsigned long game_updates) {
    if (!screen || !world) return;
    const uint32_t grid=graphics_make_color(57,96,90,255);
    const uint32_t border=graphics_make_color(101,190,130,255);
    const uint32_t avatar=graphics_make_color(255,208,111,255);
    const uint32_t landmark =
        (world->collided_this_step || world->contact_flash_steps > 0 ||
         world->ejection_flash_steps > 0)
          ? graphics_make_color(255,104,66,255)
          : graphics_make_color(132,177,239,255);
    graphics_fill_screen(screen, graphics_make_color(13,21,29,255));
    /* Flat floor, no imported stage geometry. Keep under 100 line draws. */
    for(int i=-8;i<=8;i+=2) {
        line3d(screen,world,(CK64Vec3){(float)i,0.0f,-8.0f},
                           (CK64Vec3){(float)i,0.0f,8.0f},grid);
        line3d(screen,world,(CK64Vec3){-8.0f,0.0f,(float)i},
                           (CK64Vec3){8.0f,0.0f,(float)i},grid);
    }
    /* World boundaries and an unrelated reference pillar. */
    line3d(screen,world,(CK64Vec3){-8,0,-8},(CK64Vec3){8,0,-8},border);
    line3d(screen,world,(CK64Vec3){8,0,-8},(CK64Vec3){8,0,8},border);
    line3d(screen,world,(CK64Vec3){8,0,8},(CK64Vec3){-8,0,8},border);
    line3d(screen,world,(CK64Vec3){-8,0,8},(CK64Vec3){-8,0,-8},border);
    box3d(screen,world,CK64_TEST_PILLAR_X,0,CK64_TEST_PILLAR_Z,
          CK64_TEST_PILLAR_HALF,CK64_TEST_PILLAR_HEIGHT,landmark);
    /* This box is NOT the Corn Kidz protagonist model. */
    box3d(screen,world,world->position.x,world->position.y,
          world->position.z,CK64_TEST_AVATAR_HALF,
          CK64_TEST_AVATAR_HEIGHT,avatar);
    /* Timed labels remain visible for multiple fixed updates even on
     * 60 Hz render cycles; they do not indicate original game impacts. */
    if (world->ejection_flash_steps > 0) {
        graphics_draw_box(screen,105,45,110,15,
                          graphics_make_color(101,43,34,255));
        graphics_draw_text(screen,123,48,"EJECTED");
    } else if (world->collided_this_step ||
               world->contact_flash_steps > 0) {
        graphics_draw_box(screen,105,45,110,15,
                          graphics_make_color(101,43,34,255));
        graphics_draw_text(screen,123,48,"CONTACT!");
    }
    /* Backplate keeps the control legend legible over the grid. */
    graphics_draw_box(screen,0,0,320,32,graphics_make_color(8,15,22,255));
    graphics_draw_text(screen,6,4,"CK64 N64 - PHASE 11 CONTACT HUD");
    graphics_draw_text(screen,6,18,"STICK: MOVE  A: JUMP  B: RESET");
    graphics_draw_box(screen,0,211,320,29,graphics_make_color(8,15,22,255));
    graphics_draw_text(screen,6,215,world->collision_enabled
        ? "C-L/R CAM  START COLL:ON  Z:DIAG"
        : "C-L/R CAM  START COLL:OFF Z:DIAG");
    char status[75];
    snprintf(status,sizeof(status),"X:%+.1f Z:%+.1f Y:%.1f H:%u E:%u",
             world->position.x,world->position.z,world->position.y,
             world->collision_hits,world->contact_events);
    graphics_draw_text(screen,6,228,status);
    (void)game_updates; /* Frame count visible in the Z diagnostic view. */
}
