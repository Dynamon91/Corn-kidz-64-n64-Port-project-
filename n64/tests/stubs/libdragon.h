#ifndef LIBDRAGON_H
#define LIBDRAGON_H
/* HOST-ONLY compilation stub. Never include in Nintendo 64 builds. */
#include <stdbool.h>
#include <stdint.h>
typedef struct surface_s surface_t;
typedef struct joypad_buttons_s { unsigned a:1,b:1,start:1,z:1,c_right:1,c_left:1,r:1; } joypad_buttons_t;
typedef struct joypad_inputs_s {joypad_buttons_t btn; int8_t stick_x,stick_y;} joypad_inputs_t;
#define JOYPAD_PORT_1 0
#define TICKS_PER_SECOND 46875000u
#define RESOLUTION_320x240 1
#define DEPTH_16_BPP 1
#define GAMMA_NONE 0
#define FILTERS_RESAMPLE 1
void timer_init(void);
long long timer_ticks(void);
void joypad_init(void);
void joypad_poll(void);
bool joypad_is_connected(int port);
joypad_buttons_t joypad_get_buttons_pressed(int port);
joypad_inputs_t joypad_get_inputs(int port);
void display_init(int mode, int bitdepth, int buffers, int gamma, int filters);
surface_t *display_get(void);
void display_show(surface_t *surface);
uint32_t graphics_make_color(int r,int g,int b,int a);
void graphics_fill_screen(surface_t *surface, uint32_t color);
void graphics_set_default_font(void);
void graphics_draw_text(surface_t *surface,int x,int y,const char *string);
void graphics_draw_box(surface_t *surface,int x,int y,int width,int height,uint32_t color);
void graphics_draw_line(surface_t *surface,int x0,int y0,int x1,int y1,uint32_t color);
#endif
