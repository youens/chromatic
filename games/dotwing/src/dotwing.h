#ifndef DOTWING_H
#define DOTWING_H
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include "art_ids.h"

#define DW_BOSS_LEFT ((160 - DW_BOSS_W * 8) / 2)

enum { DW_TITLE, DW_BUILDER, DW_HANGAR, DW_FLIGHT, DW_PAUSE, DW_SHOP,
       DW_RESULTS, DW_HELP, DW_TAKEOFF };

#define DW_SHAPES 5
#define DW_COLORS 6
#define DW_FACES 4
#define DW_GEARS 4
#define DW_UPGRADES 4
#define DW_SECTORS 4

/* Byte order matches the battery record; ally is new and defaults to 0. */
typedef struct {
  uint8_t shape, color, face, gear, configured;
  uint8_t weapon, shield, reactor, cleared;
  uint16_t tokens;
  uint8_t ally;
} DotProfile;

/* Moving objects use 8.8 fixed point whose high byte is the screen pixel
   plus 32, so everything on screen, or up to 32 pixels beyond an edge, is
   one byte and needs no shifting. Velocities are 8.8 pixels per frame. */
#define BIAS 32
#define HI(v) ((uint8_t)((uint16_t)(v) >> 8))
#define POS(px) ((uint16_t)((uint8_t)((px) + BIAS)) << 8 | 0x80u)

typedef struct { uint8_t hp; uint16_t x; int16_t vx; uint16_t y; int16_t vy; uint8_t kind, t, mode, arg, flash, cool, pad; } DwFoe;
typedef struct { uint8_t on; uint16_t x; int16_t vx; uint16_t y; int16_t vy; uint8_t kind; } DwShot;
typedef struct { uint8_t on; uint16_t x; uint16_t y; uint8_t kind, t, pad; } DwDrop;
typedef struct { uint8_t t, x, y, kind; } DwFx;

#define DW_FOES 8
#define DW_SHOTS 12
#define DW_BULLETS 24
#define DW_DROPS 6
#define DW_FXS 10

/* Dotwing owns D000-DBFF of work RAM while it runs. In the anthology this
   overlays inactive games' buffers; the stack keeps DC00-DFFF. */
extern uint8_t __at(0xD000) dw_rowbuf[32];
extern uint8_t __at(0xD020) dw_rowatt[32];
extern uint8_t __at(0xD040) dw_scratch[64];
extern uint8_t __at(0xD080) dw_hudbuf[64];     /* HUD window rows: tiles */
extern uint8_t __at(0xD0C0) dw_hudatt[64];     /* HUD window rows: attributes */
extern uint8_t __at(0xD100) dw_oam_b[160];
extern DwFoe __at(0xD1A0) dw_foes[DW_FOES];
extern DwShot __at(0xD220) dw_shots[DW_SHOTS];
extern DwShot __at(0xD2A0) dw_bullets[DW_BULLETS];
extern DwDrop __at(0xD390) dw_drops[DW_DROPS];
extern DwFx __at(0xD3C0) dw_fx[DW_FXS];
extern uint16_t __at(0xD400) dw_pal[64];      /* BG 0-31, OBJ 32-63 */
extern uint8_t __at(0xD480) dw_trail[64];     /* ally position history */
extern uint8_t __at(0xD500) dw_stream[64];    /* world streaming state */
/* The current sector's layout tables, copied from its terrain bank. */
#define DW_TB_BASE ((uint8_t *)0xD680)
#define DW_TB_STAMPS ((uint8_t *)0xD6C0)
#define DW_TB_PLACE ((uint8_t *)0xD700)
#define DW_TB_CELLS ((uint8_t *)0xD8E0)

extern DotProfile dw_profile;
extern uint8_t dw_state, dw_keys, dw_pressed, dw_muted, dw_sector, dw_hp,
  dw_invincible, dw_power, dw_energy, dw_burst, dw_boss_active, dw_combo,
  dw_victory, dw_focus, dw_menu_row, dw_dirty, dw_fade, dw_pal_dirty,
  dw_flash_mask, dw_flash_level, dw_scx, dw_scy, dw_shake, dw_allies,
  dw_new_best, dw_dim;
extern uint16_t dw_frame, dw_clock, dw_score, dw_best, dw_run_tokens, dw_seed,
  dw_stage_frame, dw_kills, dw_boss_hp, dw_boss_max;
extern int16_t dw_px, dw_py, dw_boss_x, dw_boss_y;
extern uint8_t *dw_oam;          /* sprite buffer being built this frame */
extern uint8_t dw_oam_n;         /* sprites written this frame */
extern volatile uint8_t dw_sfx_req, dw_song_req;

/* fixed.c: shared helpers */
void dw_sync(void) BANKED;
void dw_video_tick(void) BANKED;
void dw_xfer(const uint8_t *src, uint16_t dst, uint8_t blocks, uint8_t bank) BANKED;
extern uint16_t dw_missed;
void dw_pal_now(void) BANKED;
void dw_oam_begin(void) BANKED;
void dw_oam_flip(void) BANKED;
void dw_fade_to(uint8_t level) BANKED;
void dw_pal_commit(void) BANKED;
void dw_pal_commit_part(uint8_t first, uint8_t count) BANKED;
uint16_t dw_random(void) BANKED;
void dw_points(uint16_t n) BANKED;
void dw_save(void) BANKED;
void dw_load(void) BANKED;
void dw_clear_map(uint8_t tile, uint8_t attr) BANKED;
void dw_clear_win(void) BANKED;
#define DW_PAD 0x80u
void dw_number(uint8_t x, uint8_t y, uint16_t n, uint8_t width, uint8_t attr) BANKED;
void dw_wnumber(uint8_t x, uint8_t y, uint16_t n, uint8_t width, uint8_t attr) BANKED;
void dw_attr(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t attr) BANKED;

/* art.c: loaders and palettes */
void dw_load_common(void) BANKED;
void dw_load_fleet(uint8_t lab) BANKED;
void dw_load_menu_sprites(void) BANKED;
void dw_load_portrait(const uint8_t *look) BANKED;
void dw_pal_flight(uint8_t sector) BANKED;
void dw_pal_menu(const uint8_t *look) BANKED;
uint8_t dw_gear_y(uint8_t gear) BANKED;
uint8_t dw_face_y(uint8_t shape) BANKED;
void dw_title_art(void) BANKED;
void dw_title_row(uint8_t row) BANKED;
void dw_pal_swatches(void) BANKED;

/* terraina.c, terrainb.c, world.c: scenery and bosses */
void dw_terrain_a(uint8_t s) BANKED;
void dw_terrain_b(uint8_t s) BANKED;
void dw_world_begin(uint8_t sector) BANKED;
void dw_world_row(uint16_t row, uint8_t now) BANKED;
void dw_world_flat_row(uint8_t map_row) BANKED;
void dw_world_palettes(void) BANKED;
void dw_boss_tiles(uint8_t sector) BANKED;
void dw_arena_rows(uint8_t first, uint8_t count, uint8_t sector) BANKED;
void dw_pal_boss(uint8_t sector) BANKED;

/* main.c */
void dotwing_run(void) BANKED;
void dw_finish(uint8_t victory) BANKED;
void dw_bank(uint8_t fresh) BANKED;
void dw_sfx(uint8_t id) BANKED;
void dw_song(uint8_t id) BANKED;
void dw_sound_tick(void);          /* called from the VBlank handler */

/* screens.c */
void dw_screen(void) BANKED;
void dw_menu_update(void) BANKED;

/* rivals.c */
extern uint8_t dw_boss_t;
void dw_rivals_begin(void) BANKED;
void dw_waves(uint8_t step) BANKED;
void dw_foes_tick(uint8_t armed) BANKED;
void dw_boss_begin(void) BANKED;
void dw_boss_tick(void) BANKED;

/* play.c */
extern uint8_t dw_phase;
extern uint16_t dw_cam, dw_phase_t;
void dw_start(void) BANKED;
void dw_update(void) BANKED;
void dw_resume(void) BANKED;

/* Sound effects and songs. */
enum { SFX_NONE, SFX_SHOT, SFX_HIT, SFX_POP, SFX_BOOM, SFX_COIN, SFX_POWER,
       SFX_HURT, SFX_BURST, SFX_MOVE, SFX_OK, SFX_NO, SFX_BUY, SFX_WARN,
       SFX_BIGBOOM, SFX_LAUNCH, SFX_COUNT };
enum { SONG_NONE, SONG_TITLE, SONG_HANGAR, SONG_FLIGHT_A, SONG_FLIGHT_B,
       SONG_BOSS, SONG_CLEAR, SONG_VICTORY, SONG_OVER };

/* Background palettes during flight. */
#define PAL_TEXT 5
#define PAL_HUD 6
#define PAL_HUDBAR 7
/* Object palettes during flight. */
#define OP_PLANE 0
#define OP_ENERGY 1
#define OP_BULLET 2
#define OP_FOE 3
#define OP_FOE2 4
#define OP_FIRE 5
#define OP_GOLD 6
#define OP_SHADOW 7
#endif
