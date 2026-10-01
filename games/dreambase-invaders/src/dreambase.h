/* Dreambase Invaders for Game Boy Color and ModRetro Chromatic.

   The game is split so the anthology cartridge can bank it: fixed.c holds
   interrupt handlers and helpers that take pointers (it must stay in the
   fixed bank), while main.c, screens.c, play.c and art.c each fit a 16 KiB
   bank of their own. Built standalone, everything links into one 32 KiB ROM.

   Working memory lives in the upper half of WRAM bank 1 (0xD000-0xD7FF):
   two 32 x 20 shadow maps, the gameplay pools and palette buffers. The
   anthology launcher redraws this area whenever a game returns. */
#ifndef DREAMBASE_H
#define DREAMBASE_H
#include <gb/cgb.h>
#include <gb/gb.h>
#include <stdint.h>
#include "layout.h"

#ifdef ARCADE
#define DB_BANKED BANKED
#else
#define DB_BANKED
#endif

/* Shadow maps, one row per 32 bytes, copied to VRAM row by row. */
#define MAP_ROWS 20
#define MAP ((uint8_t *)0xD000)
#define ATT ((uint8_t *)0xD400)
#define PAL_TARGET ((uint16_t *)0xD680)
#define PAL_LIVE ((uint16_t *)0xD700)
#define WAVE ((int8_t *)0xD7C0)

/* Pools shared by the title's attract show and gameplay. */
#define MAXP 12
#define MAXK 16
extern uint8_t __at(0xD280) pr_on[MAXP];
extern int16_t __at(0xD28C) pr_x[MAXP];
extern int16_t __at(0xD2A4) pr_y[MAXP];
extern int16_t __at(0xD2BC) pr_vx[MAXP];
extern int16_t __at(0xD2D4) pr_vy[MAXP];
extern uint8_t __at(0xD2EC) pr_bx[MAXP];
extern uint8_t __at(0xD2F8) pr_ph[MAXP];
extern uint8_t __at(0xD304) pr_logo[MAXP];
extern uint8_t __at(0xD310) pk_life[MAXK];
extern int16_t __at(0xD320) pk_x[MAXK];
extern int16_t __at(0xD340) pk_y[MAXK];
extern int16_t __at(0xD360) pk_vx[MAXK];
extern int16_t __at(0xD380) pk_vy[MAXK];
extern uint8_t __at(0xD3A0) pk_tile[MAXK];
extern uint8_t __at(0xD3B0) pk_pal[MAXK];

/* Fixed point: positions and speeds in 1/64 pixel. */
#define FX 6

/* Screens. */
#define M_SPLASH 0
#define M_TITLE 1
#define M_HELP 2
#define M_PLAY 3
#define M_RESULTS 4
#define M_CREDITS 5

/* Palette slots: 0-7 background, 8-15 sprites. */
#define OBJ 8

/* Sprite tile numbers (OBJ bank 0 unless noted). */
#define SPR_FORM(f, frame) ((uint8_t)(((f) * 2 + (frame)) * 4))
#define SPR_LOGO_N(i) ((uint8_t)(64 + (i) * 4))
#define SPR_SPARK_N(k) ((uint8_t)(96 + (k) * 2))
#define SPR_POP_N(i) ((uint8_t)((i) * 2))

/* ---------------------------------------------------------- fixed.c */
extern uint8_t db_keys, db_pressed, db_home, db_muted, db_oam, db_fade;
extern uint8_t db_scx, db_scy, db_wx, db_wy, db_map_hi, db_rmode;
extern uint8_t db_wave_t, db_copper_t, db_band, db_shine;
extern uint16_t db_face;
extern volatile uint16_t db_pal_dirty;
extern uint8_t db_row_dirty[MAP_ROWS];
extern uint16_t db_frame;
extern const uint16_t form_pals[], logo_pals[], company_cols[], copper_cols[];
extern const uint16_t pal_stars[], pal_text[], pal_green[], pal_gold[];
extern const uint16_t pal_moon[], pal_base[], pal_flash[];

void db_raster_on(void);
void db_raster_off(void);
void db_flush_all(void);
void db_put(uint8_t x, uint8_t y, uint8_t tile, uint8_t attr) DB_BANKED;
void db_text(uint8_t x, uint8_t y, const char *s, uint8_t attr);
void db_center(uint8_t y, const char *s, uint8_t attr);
void db_tall(uint8_t x, uint8_t y, const char *s, uint8_t attr);
void db_digits(uint8_t x, uint8_t y, uint16_t v, uint8_t width, uint8_t attr) DB_BANKED;
void db_clear_rows(uint8_t from, uint8_t to) DB_BANKED;
void db_pal(uint8_t slot, const uint16_t *colors4);
uint16_t db_scale(uint16_t c, uint8_t f);
void db_pal_apply(void) DB_BANKED;
void db_pal_fill(uint8_t slot, uint16_t c) DB_BANKED;
void spr16(uint8_t tile, int16_t x, int16_t y, uint8_t attr) DB_BANKED;
void spr8(uint8_t tile, int16_t x, int16_t y, uint8_t attr) DB_BANKED;
void db_oam_end(void) DB_BANKED;
uint8_t db_rand(void) DB_BANKED;
uint8_t db_strlen(const char *s);

/* ----------------------------------------------------------- main.c */
extern uint16_t db_best, db_score;
extern uint8_t db_mode, db_diff, db_level, db_won;
void db_wait(void) DB_BANKED;
void db_fade_to(uint8_t level) DB_BANKED;
void db_screen_off(void) DB_BANKED;
void db_screen_on(void) DB_BANKED;
void sfx(uint8_t id, uint8_t arg) DB_BANKED;
void music(uint8_t song) DB_BANKED;
void beat(uint8_t note) DB_BANKED;

#define SFX_BLIP 0
#define SFX_EAT 1
#define SFX_MISS 2
#define SFX_LOST 3
#define SFX_TIER 4
#define SFX_DASH 5
#define SFX_FIRE 6
#define SFX_SELECT 7

#define SONG_NONE 0
#define SONG_TITLE 1
#define SONG_LOGO 2
#define SONG_START 3
#define SONG_LEVEL 4
#define SONG_OVER 5
#define SONG_ONEUP 6
#define SONG_CREDITS 7

/* ---------------------------------------------------------- screens.c */
void splash_enter(void) DB_BANKED;
void splash_step(void) DB_BANKED;
void title_enter(void) DB_BANKED;
void title_step(void) DB_BANKED;
void help_enter(void) DB_BANKED;
void help_step(void) DB_BANKED;
void results_enter(void) DB_BANKED;
void results_step(void) DB_BANKED;
void credits_enter(void) DB_BANKED;
void credits_step(void) DB_BANKED;

/* ------------------------------------------------------------- play.c */
void play_enter(void) DB_BANKED;
void play_step(void) DB_BANKED;

/* -------------------------------------------------------------- art.c */
void art_common(void) DB_BANKED;
void art_splash(void) DB_BANKED;
void art_title(void) DB_BANKED;
void art_base(uint8_t row0, uint8_t sky) DB_BANKED;
void art_cannon(uint8_t slot, uint8_t shown, uint8_t pal) DB_BANKED;
void art_stars(uint8_t rows_from, uint8_t rows_to) DB_BANKED;
void art_sky(uint8_t from, uint8_t to) DB_BANKED;
#endif
