/* Dot creator, persistent hangar and mission screens. */
#ifdef ARCADE
#pragma bank 27
#else
#pragma bank 2
#endif
#include "dotwing.h"
#include "art_ids.h"

#define SELECTED 4
#define DIM 5
#define GOLD 6
#define WHITE 7
#define PORTRAIT DW_SPR_PORTRAIT
#define SWATCH DW_SPR_SWATCH

static uint8_t edit[4], editing, note, note_time;
static const char * const shape_names[] = {"ORB", "CUBE", "DELTA", "PILL", "STAR"};
static const char * const color_names[] = {"PINK", "BLUE", "MINT", "GOLD", "LILAC", "CYAN"};
static const char * const face_names[] = {"BRIGHT", "COOL", "WINK", "FOCUS"};
static const char * const gear_names[] = {"NONE", "CAP", "HALO", "GOGGLES"};
static const uint8_t choices[] = {DW_SHAPES, DW_COLORS, DW_FACES, DW_GEARS};
static const uint8_t prices[] = {40, 80, 140};
static const uint16_t navy[] = {
  RGB(2, 3, 7), RGB(29, 30, 31), RGB(5, 7, 12), RGB(9, 12, 17)
};
static const uint16_t menu_colors[] = {
  RGB(2, 3, 7), RGB(10, 29, 27), RGB(7, 16, 21), RGB(30, 31, 31),
  RGB(2, 3, 7), RGB(14, 17, 23), RGB(7, 9, 15), RGB(22, 24, 29),
  RGB(2, 3, 7), RGB(31, 25, 10), RGB(21, 13, 6), RGB(31, 29, 23),
  RGB(2, 3, 7), RGB(28, 30, 31), RGB(14, 18, 23), RGB(31, 31, 31)
};

static void preview(void) {
  uint8_t old[4];
  old[0] = dw_profile.shape; old[1] = dw_profile.color;
  old[2] = dw_profile.face; old[3] = dw_profile.gear;
  if (dw_state == DW_BUILDER) {
    dw_profile.shape = edit[0]; dw_profile.color = edit[1];
    dw_profile.face = edit[2]; dw_profile.gear = edit[3];
  }
  dw_dot_art();
  dw_profile.shape = old[0]; dw_profile.color = old[1];
  dw_profile.face = old[2]; dw_profile.gear = old[3];
}

static void portrait(uint8_t x, uint8_t y) {
  uint8_t col, row, id = 0;
  for (col = 0; col < 4; ++col)
    for (row = 0; row < 2; ++row) {
      dw_sprite(id++, PORTRAIT + col * 4u + row * 2u,
                x + col * 8u, y + row * 16u, 0);
    }
}

static void heading(const char *name, uint8_t x) {
  dw_label(1, 1, "+", SELECTED);
  dw_label(x, 1, name, WHITE);
  dw_label(18, 1, "+", SELECTED);
  dw_label(1, 2, "------------------", DIM);
}

static void editor_enter(void) {
  edit[0] = dw_profile.shape; edit[1] = dw_profile.color;
  edit[2] = dw_profile.face; edit[3] = dw_profile.gear;
  dw_menu_row = 0;
  editing = 1;
  dw_state = DW_BUILDER;
  dw_screen();
}

static uint8_t upgrade_level(uint8_t row) {
  if (!row) return dw_profile.weapon;
  if (row == 1) return dw_profile.shield;
  return dw_profile.reactor;
}

static void upgrade_add(uint8_t row) {
  if (!row) ++dw_profile.weapon;
  else if (row == 1) ++dw_profile.shield;
  else ++dw_profile.reactor;
}

static void draw_title(void) {
  uint8_t x, y;
  /* Two rows of hand-drawn 8x8 logo tiles. */
  for (y = 0; y < DW_LOGO_HEIGHT; ++y)
    for (x = 0; x < DW_LOGO_WIDTH; ++x) {
      set_bkg_tile_xy(x + 4u, y + 2u, DW_TILE_LOGO + y * DW_LOGO_WIDTH + x);
    }
  VBK_REG = 1;
  fill_bkg_rect(4, 2, 12, 2, SELECTED);
  VBK_REG = 0;
  dw_label(1, 4, "AI AIR SUPERIORITY", DIM);
  portrait(64, 48);
  dw_label(1, 11, "YOUR DOT. YOUR SKY.", WHITE);
  dw_label(1, 13, "A/START  PLAY", SELECTED);
  dw_label(1, 14, "B        BUILD DOT", WHITE);
  dw_label(1, 15, "UP       GUIDE", DIM);
  dw_label(1, 17, "BEST", DIM);
  dw_number(6, 17, dw_best, 5, GOLD);
  dw_label(12, 17, "TOK", DIM);
  dw_number(15, 17, dw_profile.tokens, 5, GOLD);
}

static void draw_builder(void) {
  uint8_t i;
  heading("DOT BUILDER", 4);
  dw_label(2, 3, "MEET YOUR CO-PILOT", DIM);
  portrait(64, 32);
  for (i = 0; i < DW_COLORS; ++i)
    dw_sprite(8u + i, SWATCH, 56u + i * 8u, 60, 1u + i);
  dw_label(7u + edit[1], 9, "+", GOLD);
  dw_label(3, 10, "SHAPE", dw_menu_row == 0 ? SELECTED : DIM);
  dw_label(3, 11, "COLOR", dw_menu_row == 1 ? SELECTED : DIM);
  dw_label(3, 12, "FACE", dw_menu_row == 2 ? SELECTED : DIM);
  dw_label(3, 13, "GEAR", dw_menu_row == 3 ? SELECTED : DIM);
  dw_label(11, 10, shape_names[edit[0]], WHITE);
  dw_label(11, 11, color_names[edit[1]], WHITE);
  dw_label(11, 12, face_names[edit[2]], WHITE);
  dw_label(11, 13, gear_names[edit[3]], WHITE);
  dw_label(1, 10u + dw_menu_row, ">", SELECTED);
  dw_label(1, 15, "< > EDIT  DPAD ROW", DIM);
  dw_label(1, 16, "A/START SAVE B BACK", SELECTED);
  dw_label(2, 17, "SAVED ON CARTRIDGE", DIM);
}

static void draw_upgrades(void) {
  uint8_t i, level, pal;
  heading(dw_state == DW_SHOP ? "SKY STATION" : "HANGAR", dw_state == DW_SHOP ? 4 : 7);
  portrait(16, 24);
  dw_label(7, 4, dw_state == DW_SHOP ? "SECTOR CLEAR" : "PILOT ONLINE", SELECTED);
  dw_label(7, 6, "TOKENS", DIM);
  dw_number(14, 6, dw_profile.tokens, 5, GOLD);
  dw_label(3, 8, "UPGRADE  LV COST", DIM);
  for (i = 0; i < 3; ++i) {
    pal = i == dw_menu_row ? SELECTED : WHITE;
    level = upgrade_level(i);
    dw_label(3, 10u + i, i == 0 ? "WING GUN" : i == 1 ? "SHIELD" : "REACTOR", pal);
    dw_number(12, 10u + i, level, 1, pal);
    if (level < 3) dw_number(15, 10u + i, prices[level], 3, GOLD);
    else dw_label(15, 10u + i, "MAX", GOLD);
  }
  dw_label(1, 10u + dw_menu_row, ">", SELECTED);
  if (note == 1) dw_label(2, 14, "UPGRADE INSTALLED!", GOLD);
  else if (note == 2) dw_label(2, 14, "NEED MORE TOKENS", GOLD);
  else if (note == 3) dw_label(2, 14, "ALREADY MAX LEVEL", GOLD);
  else dw_label(2, 14, dw_menu_row == 0 ? "WIDER SHOT PATTERN" :
                dw_menu_row == 1 ? "MORE HULL ARMOR" : "FASTER BURST REGEN", DIM);
  if (dw_state == DW_SHOP) {
    dw_label(1, 16, "A BUY  START NEXT", SELECTED);
    dw_label(2, 17, "YOUR RUN CONTINUES", DIM);
  } else {
    dw_label(1, 16, "A BUY   B BUILD DOT", WHITE);
    dw_label(1, 17, "START  LAUNCH", SELECTED);
  }
}

static void draw_results(void) {
  heading(dw_victory ? "AI SUPREMACY" : "SORTIE ENDED", 4);
  portrait(64, 24);
  dw_label(2, 8, dw_victory ? "YOU OWN THE SKIES!" : "REBUILD. RELOAD.", SELECTED);
  dw_label(3, 10, "SCORE", DIM);
  dw_number(12, 10, dw_score, 5, WHITE);
  dw_label(3, 11, "BEST", DIM);
  dw_number(12, 11, dw_best, 5, GOLD);
  dw_label(3, 12, "RIVALS", DIM);
  dw_number(12, 12, dw_kills, 5, WHITE);
  dw_label(3, 13, "TOKENS", DIM);
  dw_number(12, 13, dw_run_tokens, 5, GOLD);
  dw_label(1, 16, "A/START  HANGAR", SELECTED);
  dw_label(1, 17, "B        TITLE", DIM);
}

static void draw_help(void) {
  heading("FLIGHT GUIDE", 4);
  dw_label(1, 4, "DPAD   STEER", WHITE);
  dw_label(1, 5, "A      FOCUS / SLOW", WHITE);
  dw_label(1, 6, "B      TOKEN BURST", GOLD);
  dw_label(1, 7, "START  PAUSE", WHITE);
  dw_label(1, 8, "SELECT SOUND", DIM);
  dw_label(1, 10, "YOUR GUN AUTO-FIRES", SELECTED);
  dw_label(1, 12, "TOKEN BOOST = POWER", GOLD);
  dw_label(1, 13, "CHAIN KILLS. WIN.", WHITE);
  dw_label(1, 14, "FOUR LABS. ONE SKY.", DIM);
  dw_label(1, 16, "A / B / START  BACK", SELECTED);
}

static void draw_pause(void) {
  heading("PAUSED", 7);
  portrait(64, 32);
  dw_label(2, 10, "TAKE A BREATHER.", WHITE);
  dw_label(2, 13, "START  RESUME", SELECTED);
  dw_label(2, 15, "B      END SORTIE", DIM);
}

void dw_screen(void) BANKED {
  if (dw_state == DW_FLIGHT || dw_state == DW_TAKEOFF) return;
  if (dw_state == DW_BUILDER && !editing) {
    edit[0] = dw_profile.shape; edit[1] = dw_profile.color;
    edit[2] = dw_profile.face; edit[3] = dw_profile.gear;
    editing = 1;
  }
  dw_clear(0);
  set_bkg_palette(0, 1, navy);
  set_bkg_palette(4, 4, menu_colors);
  preview();
  switch (dw_state) {
    case DW_TITLE: draw_title(); break;
    case DW_BUILDER: draw_builder(); break;
    case DW_HANGAR: case DW_SHOP: draw_upgrades(); break;
    case DW_RESULTS: draw_results(); break;
    case DW_HELP: draw_help(); break;
    case DW_PAUSE: draw_pause(); break;
  }
  dw_dirty = 0;
  SHOW_BKG;
  SHOW_SPRITES;
  DISPLAY_ON;
}

static void enter_hangar(void) {
  dw_state = DW_HANGAR;
  dw_menu_row = 0;
  note = note_time = 0;
  editing = 0;
  dw_screen();
}

static void buy(void) {
  uint8_t level = upgrade_level(dw_menu_row);
  if (level == 3) note = 3;
  else if (dw_profile.tokens < prices[level]) note = 2;
  else {
    dw_profile.tokens -= prices[level];
    upgrade_add(dw_menu_row);
    dw_save();
    note = 1;
    dw_tone(1880, 0x92);
  }
  if (note != 1) dw_tone(1100, 0x61);
  note_time = 80;
  dw_screen();
}

void dw_menu_update(void) BANKED {
  uint8_t count;
  switch (dw_state) {
    case DW_TITLE:
      if (dw_pressed & (J_A | J_START)) {
        if (dw_profile.configured) enter_hangar();
        else editor_enter();
      } else if (dw_pressed & J_B) editor_enter();
      else if (dw_pressed & (J_UP | J_DOWN)) {
        dw_state = DW_HELP;
        dw_screen();
      }
      break;
    case DW_BUILDER:
      if (dw_pressed & (J_A | J_START)) {
        dw_profile.shape = edit[0]; dw_profile.color = edit[1];
        dw_profile.face = edit[2]; dw_profile.gear = edit[3];
        dw_profile.configured = 1;
        dw_save();
        dw_tone(1860, 0xA2);
        enter_hangar();
      } else if (dw_pressed & J_B) {
        editing = 0;
        dw_state = DW_TITLE;
        dw_screen();
      } else {
        if (dw_pressed & J_UP) dw_menu_row = (dw_menu_row + 3u) & 3u;
        if (dw_pressed & J_DOWN) dw_menu_row = (dw_menu_row + 1u) & 3u;
        count = choices[dw_menu_row];
        if (dw_pressed & J_LEFT) edit[dw_menu_row] = edit[dw_menu_row] ? edit[dw_menu_row] - 1u : count - 1u;
        if (dw_pressed & J_RIGHT) edit[dw_menu_row] = (edit[dw_menu_row] + 1u) % count;
        if (dw_pressed & (J_UP | J_DOWN | J_LEFT | J_RIGHT)) {
          dw_tone(1700, 0x52);
          dw_screen();
        }
      }
      break;
    case DW_HANGAR: case DW_SHOP:
      if (dw_pressed & J_START) {
        note = note_time = editing = 0;
        dw_start();
      } else if ((dw_pressed & J_B) && dw_state == DW_HANGAR) editor_enter();
      else if (dw_pressed & J_A) buy();
      else if (dw_pressed & (J_UP | J_DOWN)) {
        dw_menu_row = (dw_menu_row + ((dw_pressed & J_UP) ? 2u : 1u)) % 3u;
        note = note_time = 0;
        dw_screen();
      }
      break;
    case DW_RESULTS:
      if (dw_pressed & (J_A | J_START)) enter_hangar();
      else if (dw_pressed & J_B) { dw_state = DW_TITLE; dw_screen(); }
      break;
    case DW_HELP:
      if (dw_pressed & (J_A | J_B | J_START)) { dw_state = DW_TITLE; dw_screen(); }
      break;
    case DW_PAUSE:
      if (dw_pressed & J_START) {
        dw_state = DW_FLIGHT;
        dw_clear(0);
        dw_sky(dw_sector);
        dw_palettes(dw_sector);
        dw_dirty = 1;
        dw_hud();
        dw_draw();
        NR51_REG = dw_muted ? 0 : 0xFF;
        SHOW_BKG;
        SHOW_SPRITES;
        DISPLAY_ON;
      } else if (dw_pressed & J_B) dw_finish(0);
      break;
  }
}

void dw_menu_animate(void) BANKED {
  uint8_t y;
  if (dw_state == DW_FLIGHT || dw_state == DW_TAKEOFF || dw_state == DW_HELP) return;
  if (note_time && !--note_time) {
    note = 0;
    if (dw_state == DW_HANGAR || dw_state == DW_SHOP) dw_screen();
  }
  if (dw_clock & 7u) return;
  if (dw_state == DW_TITLE) y = 48;
  else if (dw_state == DW_BUILDER || dw_state == DW_PAUSE) y = 32;
  else y = 24;
  if ((dw_clock & 63u) < 32u) ++y;
  portrait(dw_state == DW_HANGAR || dw_state == DW_SHOP ? 16 : 64, y);
}
