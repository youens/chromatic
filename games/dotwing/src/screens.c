/* Title, Dot builder, hangar and sky station, results and the guide.
   Screens change behind a palette fade with the LCD left on; cursor moves
   and edits rewrite only the tiles they touch. */
#ifndef ARCADE
#pragma bank 2
#endif
#include "dotwing.h"
#include <string.h>

#define P_BACK 0
#define P_PANEL 1
#define P_WHITE 2
#define P_GOLD 3
#define P_SEL 4
#define P_DIM 5
#define P_ICON_A 6
#define P_ICON_B 7
#define BANK1 0x08

static uint8_t edit[4], note, note_time, tally;
static uint16_t shown_score;
static const char * const shape_names[] = {"ORB", "CUBE", "DELTA", "PILL", "STAR"};
static const char * const color_names[] = {"PINK", "BLUE", "MINT", "GOLD", "LILAC", "CYAN"};
static const char * const face_names[] = {"BRIGHT", "COOL", "WINK", "FOCUS"};
static const char * const gear_names[] = {"NONE", "CAP", "HALO", "GOGGLES"};
static const char * const option_names[] = {"SHAPE", "COLOR", "FACE", "GEAR"};
static const uint8_t choices[] = {DW_SHAPES, DW_COLORS, DW_FACES, DW_GEARS};
static const char * const upgrade_names[] = {"WING GUN", "HULL", "REACTOR", "ALLY DOTS"};
static const char * const upgrade_notes[] = {
  "WIDER FASTER FIRE", "ONE MORE HEART", "BURSTS + MAGNET", "WING DOTS JOIN IN"};
static const uint8_t prices[] = {40, 80, 140};

/* --------------------------------------------------------------- helpers */
static int8_t sin_small(uint8_t t) {
  static const int8_t wave[16] = {0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3, -3, -3, -2, -1};
  return wave[(t >> 2) & 15u];
}

static void spr(int16_t x, int16_t y, uint8_t tile, uint8_t attr) {
  uint8_t *p;
  if (dw_oam_n >= 40 || x <= -8 || x >= 168 || y <= -16 || y >= 160) return;
  p = dw_oam + (dw_oam_n++ << 2);
  p[0] = (uint8_t)(y + 16);
  p[1] = (uint8_t)(x + 8);
  p[2] = tile;
  p[3] = attr;
}

static void spr_block(int16_t x, int16_t y, uint8_t tile, uint8_t cols, uint8_t rows, uint8_t attr) {
  uint8_t r, c;
  for (r = 0; r < rows; ++r)
    for (c = 0; c < cols; ++c)
      spr(x + c * 8, y + r * 16, tile + r * cols * 2u + c * 2u, attr);
}

static void text(uint8_t x, uint8_t y, const char *s, uint8_t pal) {
  uint8_t n = (uint8_t)strlen(s);
  set_bkg_tiles(x, y, n, 1, (const uint8_t *)s);
  dw_attr(x, y, n, 1, pal);
}

static void centre(uint8_t y, const char *s, uint8_t pal) {
  text((20u - (uint8_t)strlen(s)) >> 1, y, s, pal);
}

static void blank(uint8_t x, uint8_t y, uint8_t w) {
  fill_bkg_rect(x, y, w, 1, DW_T_PANEL_C);
  dw_attr(x, y, w, 1, P_PANEL);
}

static void tile(uint8_t x, uint8_t y, uint8_t t, uint8_t attr) {
  VBK_REG = 1;
  set_bkg_tile_xy(x, y, attr);
  VBK_REG = 0;
  set_bkg_tile_xy(x, y, t);
}

static void panel(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  uint8_t x1 = x + w - 1u, y1 = y + h - 1u;
  fill_bkg_rect(x + 1u, y, w - 2u, 1, DW_T_PANEL_T);
  fill_bkg_rect(x + 1u, y1, w - 2u, 1, DW_T_PANEL_B);
  fill_bkg_rect(x, y + 1u, 1, h - 2u, DW_T_PANEL_L);
  fill_bkg_rect(x1, y + 1u, 1, h - 2u, DW_T_PANEL_L);
  fill_bkg_rect(x + 1u, y + 1u, w - 2u, h - 2u, DW_T_PANEL_C);
  dw_attr(x, y, w, h, P_PANEL);
  dw_attr(x1, y + 1u, 1, h - 2u, P_PANEL | S_FLIPX);
  tile(x, y, DW_T_PANEL_TL, P_PANEL);
  tile(x1, y, DW_T_PANEL_TL, P_PANEL | S_FLIPX);
  tile(x, y1, DW_T_PANEL_BL, P_PANEL);
  tile(x1, y1, DW_T_PANEL_BL, P_PANEL | S_FLIPX);
}

static void emblem(uint8_t x, uint8_t y, uint8_t lab, uint8_t lit) {
  uint8_t t = DW_T_EMBLEM0 + lab * 4u;
  uint8_t pal = lit ? (lab < 2 ? P_ICON_A : P_ICON_B) : P_DIM;
  tile(x, y, t, pal);
  tile(x + 1u, y, t + 1u, pal);
  tile(x, y + 1u, t + 2u, pal);
  tile(x + 1u, y + 1u, t + 3u, pal);
}

/* The Dot on show: the builder's unsaved edit, otherwise the profile. */
static const uint8_t *look(void) {
  return dw_state == DW_BUILDER ? (const uint8_t *)edit : (const uint8_t *)&dw_profile;
}

static void preview(void) {
  dw_load_portrait(look());
  dw_pal_menu(look());
  if (dw_state == DW_BUILDER) dw_pal_swatches();
}

static void portrait(int16_t x, int16_t y) {
  const uint8_t *dot = look();
  uint8_t fy = dw_face_y(dot[0]), gear = dot[3];
  if (gear) spr_block(x, y + (int8_t)dw_gear_y(gear), DW_M_GEAR, 4, 1, 2 | BANK1);
  spr_block(x + 8, y + fy, DW_M_FACE, 2, 1, 1 | BANK1);
  spr_block(x, y, DW_M_PORTRAIT, 4, 2, 0 | BANK1);
}

static void hero(int16_t x, int16_t y) {
  spr_block(x, y, DW_M_HERO_GLASS, 4, 2, 4 | BANK1);
  spr_block(x, y, DW_M_HERO, 4, 2, 3 | BANK1);
  spr(x + 12, y + 30, (dw_clock & 2u) ? DW_S_FLAME1 : DW_S_FLAME, 7);
}

static void sparkles(void) {
  static const uint8_t sx[6] = {12, 140, 30, 120, 76, 150};
  static const uint8_t sy[6] = {8, 20, 60, 70, 4, 100};
  uint8_t i, phase = (uint8_t)(dw_clock >> 3);
  for (i = 0; i < 6; ++i)
    if (((phase + i * 3u) & 7u) < 3u) spr(sx[i], sy[i], DW_M_SPARKLE, 6 | BANK1);
}

/* ------------------------------------------------------------ the title */
static void draw_title(void) {
  dw_title_art();
  centre(DW_TITLE_TEXT_ROW, "PRESS START", 5);
  text(1, DW_TITLE_INFO_ROW, "HI", 3);
  dw_number(4, DW_TITLE_INFO_ROW, dw_best, 5, 4);
  text(13, DW_TITLE_INFO_ROW, "$", 4);
  dw_number(14, DW_TITLE_INFO_ROW, dw_profile.tokens, 5 | DW_PAD, 4);
}

static void title_sprites(void) {
  int16_t bob = sin_small((uint8_t)(dw_clock >> 1));
  hero(64, 52 + bob);
  spr_block(176 - (int16_t)((dw_clock >> 1) % 220u), 70, DW_M_CLOUD, 4, 1, 5 | BANK1);
  spr_block(190 - (int16_t)(dw_clock % 230u), 84, DW_M_CLOUD_SMALL, 2, 1, 5 | BANK1);
  spr_block(240 - (int16_t)((dw_clock >> 2) % 280u), 62, DW_M_CLOUD_SMALL, 2, 1, 5 | BANK1);
  sparkles();
}

/* ---------------------------------------------------------- the builder */
static void builder_row(uint8_t row) {
  uint8_t y = 11u + row, sel = row == dw_menu_row;
  const char *value;
  blank(1, y, 18);
  text(2, y, option_names[row], sel ? P_SEL : P_DIM);
  if (row == 0) value = shape_names[edit[0]];
  else if (row == 1) value = color_names[edit[1]];
  else if (row == 2) value = face_names[edit[2]];
  else value = gear_names[edit[3]];
  text(10, y, value, sel ? P_WHITE : P_DIM);
  if (sel) {
    tile(8, y, '[', P_SEL);
    tile(18, y, ']', P_SEL);
  }
}

static void draw_builder(void) {
  uint8_t i;
  panel(0, 0, 20, 10);
  centre(1, "DOT BUILDER", P_GOLD);
  panel(0, 10, 20, 8);
  for (i = 0; i < 4; ++i) builder_row(i);
  text(2, 16, "A SAVE  B CANCEL", P_SEL);
}

static void builder_sprites(void) {
  uint8_t i;
  int16_t bob = sin_small((uint8_t)dw_clock) >> 1;
  portrait(64, 24 + bob);
  for (i = 0; i < DW_COLORS; ++i)
    spr(32 + i * 16, 64, DW_M_SWATCH1 + (i % 3u) * 2u, (i < 3 ? 5 : 7) | BANK1);
  if (dw_clock & 8u) spr(32 + edit[1] * 16, 64, DW_M_RING, 4 | BANK1);
  sparkles();
}

/* ------------------------------------------------- hangar and sky station */
static uint8_t level_of(uint8_t row) {
  return row == 0 ? dw_profile.weapon : row == 1 ? dw_profile.shield :
         row == 2 ? dw_profile.reactor : dw_profile.ally;
}

static void upgrade_row(uint8_t row) {
  uint8_t y = 9u + row, sel = row == dw_menu_row, level = level_of(row), i;
  blank(1, y, 18);
  text(2, y, upgrade_names[row], sel ? P_SEL : P_WHITE);
  for (i = 0; i < 3; ++i) tile(11 + i, y, i < level ? '#' : '&', P_GOLD);
  if (level < 3) {
    tile(15, y, '$', P_GOLD);
    dw_number(16, y, prices[level], 3 | DW_PAD, P_GOLD);
  } else {
    text(15, y, "MAX", P_DIM);
  }
}

static void upgrade_note(void) {
  blank(1, 14, 18);
  if (note == 1) text(2, 14, "INSTALLED!", P_GOLD);
  else if (note == 2) text(2, 14, "NEED MORE TOKENS", P_GOLD);
  else if (note == 3) text(2, 14, "ALREADY MAXED", P_GOLD);
  else text(2, 14, upgrade_notes[dw_menu_row], P_DIM);
}

static void tokens_line(void) {
  tile(6, 4, '$', P_GOLD);
  dw_number(7, 4, dw_profile.tokens, 5 | DW_PAD, P_GOLD);
}

static void draw_upgrades(void) {
  uint8_t i;
  panel(0, 0, 20, 8);
  if (dw_state == DW_SHOP) {
    text(7, 1, "SKY STATION", P_GOLD);
    text(7, 2, "SECTOR", P_SEL);
    tile(14, 2, '0' + dw_sector, P_SEL);
    text(16, 2, "\\", P_SEL);
  } else {
    text(9, 1, "HANGAR", P_GOLD);
    text(7, 2, "PILOT READY", P_SEL);
  }
  tokens_line();
  text(13, 4, "TOKENS", P_DIM);
  for (i = 0; i < 4; ++i) emblem(7 + i * 3u, 5, i, dw_profile.cleared > i);
  panel(0, 8, 20, 10);
  for (i = 0; i < 4; ++i) upgrade_row(i);
  upgrade_note();
  if (dw_state == DW_SHOP) {
    text(2, 16, "A BUY  START NEXT", P_SEL);
  } else {
    text(2, 16, "A BUY  B PILOT", P_WHITE);
    text(2, 15, "START  LAUNCH", P_SEL);
  }
}

static void upgrade_sprites(void) {
  int16_t bob = sin_small((uint8_t)dw_clock) >> 1;
  hero(16, 16 + bob);
  spr(3 + ((dw_clock >> 3) & 1u), 72 + dw_menu_row * 8, DW_M_CURSOR, 4 | BANK1);
}

/* -------------------------------------------------------------- results */
static void draw_results(void) {
  panel(0, 0, 20, 18);
  centre(1, dw_victory ? "AI SUPREMACY!" : "SORTIE OVER", P_GOLD);
  centre(7, dw_victory ? "YOU OWN THE SKY" : "TOKENS BANKED", P_SEL);
  text(3, 9, "SCORE", P_DIM);
  dw_number(11, 9, 0, 6 | DW_PAD, P_WHITE);
  text(3, 10, "BEST", P_DIM);
  dw_number(11, 10, dw_best, 6 | DW_PAD, dw_new_best ? P_GOLD : P_WHITE);
  text(3, 11, "RIVALS", P_DIM);
  dw_number(11, 11, dw_kills, 6 | DW_PAD, P_WHITE);
  text(3, 12, "TOKENS", P_DIM);
  dw_number(11, 12, dw_run_tokens, 6 | DW_PAD, P_GOLD);
  text(3, 13, "SECTOR", P_DIM);
  dw_number(11, 13, dw_victory ? 4 : dw_sector, 6 | DW_PAD, P_WHITE);
  if (dw_new_best) centre(15, "NEW RECORD!", P_GOLD);
  text(2, 16, "A HANGAR  B TITLE", P_SEL);
  shown_score = 0;
  tally = 1;
}

/* ---------------------------------------------------------------- guide */
static void draw_help(void) {
  panel(0, 0, 20, 18);
  centre(1, "FLIGHT GUIDE", P_GOLD);
  text(2, 3, "DPAD  FLY", P_WHITE);
  text(2, 4, "A     FOCUS", P_WHITE);
  text(2, 5, "B     BURST", P_WHITE);
  text(2, 6, "START PAUSE", P_WHITE);
  text(2, 8, "GUNS AUTO-FIRE.", P_DIM);
  text(4, 10, "TOKENS", P_GOLD);
  text(4, 11, "MORE GUNS", P_WHITE);
  text(4, 12, "+1 HEART", P_WHITE);
  text(4, 13, "TURBO", P_WHITE);
  text(4, 14, "WING DOT", P_WHITE);
  centre(16, "PRESS A", P_SEL);
}

static void help_sprites(void) {
  spr(16, 80, DW_S_COIN + ((dw_clock >> 2) & 3u) * 2u, 6);
  spr(16, 88, DW_S_POWER, 6);
  spr(16, 96, DW_S_REPAIR, 7);
  spr(16, 104, DW_S_BOLT, 1);
  spr(16, 112, (dw_clock & 4u) ? DW_S_ALLY1 : DW_S_ALLY, 0);
}

/* ------------------------------------------------------------- dispatch */
static void draw_sprites(void) {
  switch (dw_state) {
    case DW_TITLE: title_sprites(); break;
    case DW_BUILDER: builder_sprites(); break;
    case DW_HANGAR: case DW_SHOP: upgrade_sprites(); break;
    case DW_RESULTS: portrait(64, 22 + (sin_small((uint8_t)dw_clock) >> 1)); break;
    case DW_HELP: help_sprites(); break;
  }
}

void dw_screen(void) BANKED {
  HIDE_WIN;
  dw_dim = 0;
  dw_flash_mask = 0;
  dw_shake = 0;
  dw_scx = dw_scy = 0;
  SCX_REG = SCY_REG = 0;
  note = note_time = tally = 0;
  if (dw_state == DW_BUILDER) memcpy(edit, &dw_profile, 4);
  dw_clear_map(DW_T_BLANK, P_BACK);
  preview();
  switch (dw_state) {
    case DW_TITLE: draw_title(); break;
    case DW_BUILDER: draw_builder(); break;
    case DW_HANGAR: case DW_SHOP: draw_upgrades(); break;
    case DW_RESULTS: draw_results(); break;
    case DW_HELP:
      draw_help();
      dw_pal_flight(1);
      break;
  }
  dw_oam_begin();
  draw_sprites();
  dw_oam_flip();
  LCDC_REG |= LCDCF_ON;
  dw_fade_to(8);
}

static void go(uint8_t state) {
  dw_fade_to(0);
  dw_state = state;
  if (state == DW_TITLE) dw_song(SONG_TITLE);
  else if (state == DW_BUILDER || state == DW_HANGAR || state == DW_HELP) dw_song(SONG_HANGAR);
  dw_screen();
}

static void buy(void) {
  uint8_t level = level_of(dw_menu_row);
  if (level == 3) note = 3;
  else if (dw_profile.tokens < prices[level]) note = 2;
  else {
    dw_profile.tokens -= prices[level];
    if (dw_menu_row == 0) ++dw_profile.weapon;
    else if (dw_menu_row == 1) ++dw_profile.shield;
    else if (dw_menu_row == 2) ++dw_profile.reactor;
    else ++dw_profile.ally;
    dw_save();
    note = 1;
    dw_sfx(SFX_BUY);
    upgrade_row(dw_menu_row);
    tokens_line();
  }
  if (note != 1) dw_sfx(SFX_NO);
  note_time = 90;
  upgrade_note();
}

void dw_menu_update(void) BANKED {
  uint8_t count, old;
  switch (dw_state) {
    case DW_TITLE:
      if ((dw_clock & 31u) == 0) {
        dw_pal[4 * 5 + 3] = (dw_clock & 32u) ? 0x7FFF : 0x6F7B;
        dw_pal[4 * 5 + 2] = (dw_clock & 32u) ? 0x7F9B : 0x5EF7;
        dw_pal_commit();
      }
      if ((dw_clock & 255u) == 160u) {
        dw_title_row(DW_TITLE_TEXT_ROW);
        centre(DW_TITLE_TEXT_ROW, "B PILOT  UP GUIDE", 5);
      } else if ((dw_clock & 255u) == 0u) {
        dw_title_row(DW_TITLE_TEXT_ROW);
        centre(DW_TITLE_TEXT_ROW, "PRESS START", 5);
      }
      if (dw_pressed & (J_A | J_START)) {
        dw_sfx(SFX_OK);
        go(dw_profile.configured ? DW_HANGAR : DW_BUILDER);
      } else if (dw_pressed & J_B) {
        dw_sfx(SFX_OK);
        go(DW_BUILDER);
      } else if (dw_pressed & (J_UP | J_DOWN)) {
        dw_sfx(SFX_MOVE);
        go(DW_HELP);
      }
      break;
    case DW_BUILDER:
      if (dw_pressed & (J_A | J_START)) {
        memcpy(&dw_profile, edit, 4);
        dw_profile.configured = 1;
        dw_save();
        dw_sfx(SFX_OK);
        dw_menu_row = 0;
        go(DW_HANGAR);
        return;
      }
      if (dw_pressed & J_B) {
        dw_sfx(SFX_NO);
        go(dw_profile.configured ? DW_HANGAR : DW_TITLE);
        return;
      }
      old = dw_menu_row;
      if (dw_pressed & J_UP) dw_menu_row = (dw_menu_row + 3u) & 3u;
      if (dw_pressed & J_DOWN) dw_menu_row = (dw_menu_row + 1u) & 3u;
      if (old != dw_menu_row) {
        builder_row(old);
        builder_row(dw_menu_row);
        dw_sfx(SFX_MOVE);
      }
      count = choices[dw_menu_row];
      if (dw_pressed & (J_LEFT | J_RIGHT)) {
        if (dw_pressed & J_LEFT) edit[dw_menu_row] = edit[dw_menu_row] ? edit[dw_menu_row] - 1u : count - 1u;
        else edit[dw_menu_row] = (edit[dw_menu_row] + 1u) % count;
        builder_row(dw_menu_row);
        preview();
        dw_pal_commit();
        dw_sfx(SFX_MOVE);
      }
      break;
    case DW_HANGAR:
    case DW_SHOP:
      if (note_time && !--note_time) {
        note = 0;
        upgrade_note();
      }
      if (dw_pressed & J_START) {
        dw_sfx(SFX_LAUNCH);
        dw_start();
        return;
      }
      if ((dw_pressed & J_B) && dw_state == DW_HANGAR) {
        dw_sfx(SFX_OK);
        dw_menu_row = 0;
        go(DW_BUILDER);
        return;
      }
      if (dw_pressed & J_A) buy();
      else if (dw_pressed & (J_UP | J_DOWN)) {
        old = dw_menu_row;
        dw_menu_row = (dw_menu_row + ((dw_pressed & J_UP) ? 3u : 1u)) & 3u;
        note = note_time = 0;
        upgrade_row(old);
        upgrade_row(dw_menu_row);
        upgrade_note();
        dw_sfx(SFX_MOVE);
      }
      break;
    case DW_RESULTS:
      if (tally) {
        uint16_t step = (dw_score - shown_score) >> 3;
        shown_score += step ? step : (dw_score != shown_score);
        dw_number(11, 9, shown_score, 6 | DW_PAD, P_WHITE);
        if (shown_score == dw_score) tally = 0;
        else if (!(dw_clock & 3u)) dw_sfx(SFX_MOVE);
      }
      if (dw_pressed & (J_A | J_START)) {
        dw_sfx(SFX_OK);
        dw_menu_row = 0;
        go(DW_HANGAR);
      } else if (dw_pressed & J_B) {
        go(DW_TITLE);
      }
      break;
    case DW_HELP:
      if (dw_pressed & (J_A | J_B | J_START)) {
        dw_sfx(SFX_OK);
        go(DW_TITLE);
      }
      break;
  }
  if (dw_state != DW_FLIGHT && dw_state != DW_TAKEOFF) draw_sprites();
}
