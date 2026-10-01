/* Graphics loaders. Everything that reads tile or map data lives here, so
   the anthology can keep the art in a bank of its own. */
#include "dreambase.h"
#include "assets.h"
static const uint16_t star_list[] = STAR_LIST_INIT;
#include <string.h>

/* VRAM addresses (the LCD is off whenever these loaders run). */
#define OBJ_TILE(n) ((uint8_t *)(0x8000 + (uint16_t)(n) * 16))
#define BG_TILE(n) ((uint8_t *)((n) < 128 ? 0x9000 + (uint16_t)(n) * 16 : 0x8000 + (uint16_t)(n) * 16))

static uint8_t base_row;
static const uint8_t slot_col[5] = {2, 5, 9, 13, 16};
static const uint8_t zero16[16] = {0};

/* Each star_list entry packs (tile << 10) | (y << 5) | x, sorted by row. */
static void star_row(uint8_t y, uint8_t *row) {
  uint8_t i;
  uint16_t v;
  for (i = 0; i < 32; i++)
    row[i] = 0;
  for (i = 0; i < STAR_COUNT; i++) {
    v = star_list[i];
    if (((v >> 5) & 31) == y)
      row[v & 31] = T_STAR0 + (v >> 10);
  }
}

/* Copy 20-wide map rows into the shadow maps. */
static void copy_rows(uint8_t row, uint8_t rows, const uint8_t *t, const uint8_t *a) {
  uint8_t x;
  uint16_t i;
  for (; rows; rows--, row++) {
    i = (uint16_t)row << 5;
    for (x = 0; x < 20; x++) {
      MAP[i + x] = *t++;
      ATT[i + x] = *a++;
    }
    db_row_dirty[row] = 1;
  }
}

void art_common(void) DB_BANKED {
  uint8_t *buf, i, j, c;
  const uint8_t *src;
  VBK_REG = 0;
  memcpy(OBJ_TILE(0), spr_tiles, SPR_COUNT * 16);
  for (i = 0; i < SPARK_COUNT; i++) {
    memcpy(OBJ_TILE(SPR_COUNT + i * 2), spark_tiles + i * 16, 16);
    memset(OBJ_TILE(SPR_COUNT + i * 2 + 1), 0, 16);
  }
  memset(BG_TILE(0), 0, 32);
  memcpy(BG_TILE(2), font_tiles, FONT_COUNT * 16);
  memcpy(BG_TILE(60), hud_tiles, HUD_COUNT * 16);
  VBK_REG = 1;
  for (i = 0; i < POP_COUNT; i++) {
    memcpy(OBJ_TILE(i * 2), pop_tiles + i * 16, 16);
    memset(OBJ_TILE(i * 2 + 1), 0, 16);
  }
  /* Double-height letters: each font row is repeated. */
  for (i = 0; i < TALL_COUNT; i++) {
    c = TALL_CHARS[i];
    src = c == ' ' ? zero16 : font_tiles + (uint16_t)(c - 33) * 16;
    buf = BG_TILE(i * 2);
    for (j = 0; j < 8; j++, src += 2, buf += 4) {
      buf[0] = buf[2] = src[0];
      buf[1] = buf[3] = src[1];
    }
  }
  memcpy(BG_TILE(BASE_FIRST), base_tiles, BASE_COUNT * 16);
  VBK_REG = 0;
}

void art_splash(void) DB_BANKED {
  VBK_REG = 0;
  memcpy(BG_TILE(SPLASH_FIRST), splash_tiles, SPLASH_COUNT * 16);
  /* Mark at rows 2-7, wordmark at rows 9-10, all in the hidden palette. */
  const uint8_t *m = splash_map;
  uint8_t x, y;
  for (y = 0; y < 6; y++)
    for (x = 0; x < 6; x++, m += 2)
      db_put(7 + x, 2 + y, m[0], (m[1] & 0xF8) | 7);
  for (y = 0; y < 2; y++)
    for (x = 0; x < 16; x++, m += 2)
      db_put(2 + x, 9 + y, m[0], (m[1] & 0xF8) | 7);
}

/* Recolour invader tiles for the title formation: two invaders share one
   palette, one drawn in colour 2 and the other moved to colour 1. Their
   shading becomes transparent against the night sky. */
static const uint8_t formation[8] = {1, 2, 3, 5, 4, 6, 7, 0};
void art_title(void) DB_BANKED {
  uint8_t i, k, lo, hi, *buf;
  const uint8_t *src;
  VBK_REG = 0;
  memcpy(BG_TILE(TITLE_FIRST), title_tiles, TITLE_COUNT * 16);
  copy_rows(1, 6, title_map, title_attr);
  VBK_REG = 1;
  buf = OBJ_TILE(SPR_TITLE);
  for (i = 0; i < 8; i++) {
    src = spr_tiles + (uint16_t)formation[i] * 128;
    if (i & 1)
      for (k = 64; k; k--) {
        lo = *src++;
        hi = *src++;
        *buf++ = hi;
        *buf++ = lo & hi;
      }
    else
      for (k = 64; k; k--) {
        lo = *src++;
        hi = *src++;
        *buf++ = lo & hi;
        *buf++ = hi;
      }
  }
  VBK_REG = 0;
}

/* sky is the palette for the base's sky tiles: the star palette on the
   title, and a separate one in play so the sky's miss flash stays above. */
void art_base(uint8_t row0, uint8_t sky) DB_BANKED {
  uint8_t i;
  uint8_t *a;
  base_row = row0;
  copy_rows(row0, 4, base_map, base_attr);
  a = ATT + ((uint16_t)row0 << 5);
  for (i = 0; i < 96; i++, a++) {
    if ((i & 31) == 20)
      i += 11, a += 11;
    else if ((*a & 7) == 4)
      *a = (*a & 0xF8) | sky;
  }
}

/* A turret, or a closed hatch, in palette 7 (brand colour) or 3 (white). */
void art_cannon(uint8_t slot, uint8_t shown, uint8_t pal) DB_BANKED {
  const uint8_t *q = shown ? cannon_cannon : cannon_hatch;
  uint8_t k, x = slot_col[slot];
  if (slot == 2)
    return;
  for (k = 0; k < 4; k++)
    db_put(x + (k & 1), base_row + 1 + (k >> 1), q[k * 2], (q[k * 2 + 1] & 0xF8) | pal | 0x80);
}

/* The scrolling playfield: a 32 x 32 starfield written straight to VRAM.
   The whole map is drawn with the LCD off; a few rows can be restored later
   with the VRAM-safe library calls. */
void art_stars(uint8_t from, uint8_t to) DB_BANKED {
  uint8_t i, y = from, n = ((to - from) & 31) + 1, row[32], att[32];
  uint16_t v;
  if (n == 32) {
    VBK_REG = 1;
    memset((uint8_t *)0x9800, 4, 1024);
    VBK_REG = 0;
    memset((uint8_t *)0x9800, 0, 1024);
    for (i = 0; i < STAR_COUNT; i++) {
      v = star_list[i];
      *(uint8_t *)(0x9800 + (v & 0x3FF)) = T_STAR0 + (v >> 10);
    }
    return;
  }
  memset(att, 4, 32);
  for (; n; n--, y = (y + 1) & 31) {
    star_row(y, row);
    VBK_REG = 1;
    set_bkg_tiles(0, y, 32, 1, att);
    VBK_REG = 0;
    set_bkg_tiles(0, y, 32, 1, row);
  }
}

/* Sparse stars in the shadow map, for still screens. */
void art_sky(uint8_t from, uint8_t to) DB_BANKED {
  uint8_t i, x, y;
  uint16_t v;
  for (i = 0; i < STAR_COUNT; i++) {
    v = star_list[i];
    x = v & 31;
    y = (v >> 5) & 31;
    if (x < 20 && y >= from && y <= to)
      db_put(x, y, T_STAR0 + (v >> 10), 4);
  }
}
