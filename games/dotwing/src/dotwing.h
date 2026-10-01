#ifndef DOTWING_H
#define DOTWING_H
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

enum { DW_TITLE, DW_BUILDER, DW_HANGAR, DW_FLIGHT, DW_PAUSE, DW_SHOP,
       DW_RESULTS, DW_HELP, DW_TAKEOFF };
#define DW_ENEMIES 6
#define DW_SHOTS 8
#define DW_BULLETS 8
#define DW_DROPS 4
#define DW_SHAPES 5
#define DW_COLORS 6
#define DW_FACES 4
#define DW_GEARS 4

typedef struct { uint8_t shape, color, face, gear, configured;
  uint8_t weapon, shield, reactor, cleared; uint16_t tokens; } DotProfile;
typedef struct { int16_t x, y; int8_t vx, vy; uint8_t kind, hp, age, active; } DwEnemy;
typedef struct { int16_t x, y; int8_t vx, vy; uint8_t active; } DwBullet;
typedef struct { int16_t x, y; uint8_t kind, active; } DwDrop;

extern DotProfile dw_profile;
extern uint8_t dw_state, dw_keys, dw_pressed, dw_muted, dw_sector, dw_hp,
  dw_invincible, dw_power, dw_energy, dw_burst, dw_boss_hp, dw_boss_max,
  dw_boss_active, dw_combo, dw_victory, dw_focus, dw_menu_row, dw_dirty;
extern uint16_t dw_frame, dw_clock, dw_score, dw_best, dw_run_tokens, dw_seed,
  dw_stage_frame, dw_kills;
extern int16_t dw_px, dw_py, dw_boss_x, dw_boss_y;
extern DwEnemy dw_enemies[DW_ENEMIES];
extern DwBullet dw_shots[DW_SHOTS], dw_bullets[DW_BULLETS];
extern DwDrop dw_drops[DW_DROPS];

/* The bridge copies caller-bank strings before rendering in the helper bank. */
extern char __at(0xDF00) dw_text[21];
void dw_label_buffer(uint8_t x, uint8_t y, uint8_t palette) BANKED;
void dw_clear(uint8_t palette) BANKED;
void dw_label(uint8_t x, uint8_t y, const char *text, uint8_t palette);
void dw_number(uint8_t x, uint8_t y, uint16_t n, uint8_t width, uint8_t palette) BANKED;
void dw_hide(void) BANKED;
void dw_sprite(uint8_t id, uint8_t tile, int16_t x, int16_t y, uint8_t palette) BANKED;
void dw_actor(uint8_t id, uint8_t tile, int16_t x, int16_t y, uint8_t palette) BANKED;
void dw_tone(uint16_t frequency, uint8_t envelope) BANKED;
void dw_noise(uint8_t pitch) BANKED;
uint16_t dw_random(void) BANKED;
void dw_points(uint16_t n) BANKED;
void dw_save(void) BANKED;
void dw_load(void) BANKED;
void dw_finish(uint8_t victory) BANKED;

void dotwing_run(void) BANKED;
void dw_screen(void) BANKED;
void dw_menu_update(void) BANKED;
void dw_menu_animate(void) BANKED;
void dw_start(void) BANKED;
void dw_update(void) BANKED;
void dw_draw(void) BANKED;
void dw_hud(void) BANKED;
void dw_art_load(void) BANKED;
void dw_dot_art(void) BANKED;
void dw_sky(uint8_t sector) BANKED;
void dw_palettes(uint8_t sector) BANKED;
#endif
