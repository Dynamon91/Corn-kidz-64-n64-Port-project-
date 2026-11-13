/* Host-side test of the actual Phase 12 N64 CPU-wireframe renderer.
 * Stubs count drawing calls; never runs on original console.
 */
#include "wireframe.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct surface_s { int marker; };
static unsigned graphics_lines;
static char last_grid_label[80];

uint32_t graphics_make_color(int r,int g,int b,int a) {
    return (uint32_t)((unsigned)r ^ (unsigned)g ^ (unsigned)b ^ (unsigned)a);
}
void graphics_fill_screen(surface_t *screen, uint32_t color) {
    assert(screen != NULL); (void)color;
}
void graphics_draw_line(surface_t *screen,int x0,int y0,int x1,int y1,uint32_t color) {
    assert(screen != NULL);
    assert(x0 >= 0 && x0 <= 319 && y0 >= 0 && y0 <= 239);
    assert(x1 >= 0 && x1 <= 319 && y1 >= 0 && y1 <= 239);
    (void)color;
    ++graphics_lines;
}
void graphics_draw_box(surface_t *screen,int x,int y,int w,int h,uint32_t color) {
    assert(screen != NULL); (void)x; (void)y; (void)w; (void)h; (void)color;
}
void graphics_draw_text(surface_t *screen,int x,int y,const char *s) {
    assert(screen != NULL); (void)x; (void)y;
    if (strstr(s,"GRID:")) {
        assert(strlen(s) < sizeof(last_grid_label));
        strcpy(last_grid_label,s);
    }
}
static void test_grid_mode(unsigned mode,unsigned expected,const char *label) {
    struct surface_s screen = {0};
    CK64World3D world;
    CK64WireframeStats stats = {777,999};
    ck64_world3d_reset(&world);
    graphics_lines=0;
    last_grid_label[0]='\0';
    ck64_wireframe_draw(&screen,&world,0,mode,412,&stats);
    assert(stats.lines_attempted == expected);
    assert(stats.lines_drawn <= stats.lines_attempted);
    assert(graphics_lines == stats.lines_drawn);
    assert(strstr(last_grid_label,label) != NULL);
    assert(strstr(last_grid_label,"CPU:412us") != NULL);
}
int main(void) {
    /* 4 border edges + 2 solid boxes x12 edges = 28 baseline. */
    test_grid_mode(CK64_GRID_OFF,28,"GRID:OFF");
    test_grid_mode(CK64_GRID_SPARSE,38,"GRID:LOW");
    test_grid_mode(CK64_GRID_FULL,46,"GRID:FULL");
    test_grid_mode(99,46,"GRID:FULL"); /* invalid mode falls back to FULL */

    /* Early return on missing screen/world does not overwrite counters. */
    CK64World3D w; ck64_world3d_reset(&w);
    CK64WireframeStats untouched={101,202};
    struct surface_s screen = {0};
    ck64_wireframe_draw(NULL,&w,0,CK64_GRID_FULL,0,&untouched);
    assert(untouched.lines_attempted==101 && untouched.lines_drawn==202);
    ck64_wireframe_draw(&screen,NULL,0,CK64_GRID_FULL,0,&untouched);
    assert(untouched.lines_attempted==101 && untouched.lines_drawn==202);
    ck64_wireframe_draw(&screen,&w,0,CK64_GRID_OFF,0,NULL);

    puts("5 grid mode/count and 2 invalid-argument checks passed");
    return 0;
}
