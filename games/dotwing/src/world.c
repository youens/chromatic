/* Scenery streaming and background-layer bosses.

   A level is a sorted list of stamp placements. Each new map row starts as
   the sector's base pattern; every stamp crossing that row then copies its
   cells over it. Level rows count upwards; level row r lives in map row
   (31 - r) & 31, and level column c in map column (c - 2) & 31, so the
   24-column level overhangs the screen by 16 pixels on each side. */
#ifndef ARCADE
#pragma bank 6
#endif
#include "dotwing.h"
#include "boss_data.h"
#include <string.h>

#define SECTOR_PAL ((uint16_t *)0xD510)

typedef struct {
  uint8_t sector, next, count;
  uint8_t active[12];
} Stream;
#define ST ((Stream *)dw_stream)

static uint8_t flat_tile, flat_attr;

void dw_world_begin(uint8_t sector) BANKED {
  uint8_t s = (uint8_t)(sector - 1u) & 3u;
  ST->sector = s;
  ST->next = 0;
  ST->count = 0;
  if (s < 2) dw_terrain_a(s);
  else dw_terrain_b(s);
  flat_tile = DW_TB_BASE[0];
  flat_attr = DW_TB_BASE[1];
  memcpy(SECTOR_PAL, dw_pal, 48);
}

/* Restore the sector's scenery palettes after a boss borrowed 1-5. */
void dw_world_palettes(void) BANKED {
  memcpy(dw_pal, SECTOR_PAL, 48);
}

/* Arrange the 24 level columns into map order, padded with flat sky, then
   either write now (setup, screen black) or queue for the next VBlank. */
static void write_row(uint8_t map_row, uint8_t now) {
  uint8_t c;
  for (c = 23; c != 255; --c) {
    uint8_t m = (uint8_t)(c - 2u) & 31u;
    dw_scratch[m] = dw_rowbuf[c];
    dw_scratch[32 + m] = dw_rowatt[c];
  }
  for (c = 22; c < 30; ++c) {
    dw_scratch[c] = flat_tile;
    dw_scratch[32 + c] = flat_attr;
  }
  for (c = 0; c < 32; ++c) {
    dw_rowbuf[c] = dw_scratch[c];
    dw_rowatt[c] = dw_scratch[32 + c];
  }
  if (now) {
    VBK_REG = 1;
    set_bkg_tiles(0, map_row, 32, 1, dw_rowatt);
    VBK_REG = 0;
    set_bkg_tiles(0, map_row, 32, 1, dw_rowbuf);
  } else {
    uint16_t dst = 0x9800u + ((uint16_t)map_row << 5);
    dw_xfer(dw_rowbuf, dst, 2, 0);
    dw_xfer(dw_rowatt, dst, 2, 1);
  }
}

void dw_world_row(uint16_t row, uint8_t now) BANKED {
  const uint8_t *base = DW_TB_BASE;
  const uint8_t *place = DW_TB_PLACE;
  const uint8_t *stamps = DW_TB_STAMPS;
  const uint8_t *cells = DW_TB_CELLS;
  const uint8_t *p, *st, *src;
  uint8_t c, i, h, w, x, col, flip, live;
  uint16_t prow, off;
  if (row < DW_LEVEL_ROWS) {
    uint8_t r8 = (uint8_t)row;
    uint8_t mix = (uint8_t)(r8 * 7u) ^ (uint8_t)(row >> 3);
    for (c = 0; c < DW_LEVEL_WIDTH; ++c) {
      h = (uint8_t)((mix ^ (uint8_t)(c * 13u)) & 31u) << 1;
      dw_rowbuf[c] = base[h];
      dw_rowatt[c] = base[h + 1];
    }
  } else {
    for (c = 0; c < DW_LEVEL_WIDTH; ++c) {
      dw_rowbuf[c] = flat_tile;
      dw_rowatt[c] = flat_attr;
    }
  }
  /* Activate placements whose bottom row has been reached. */
  for (;;) {
    p = place + ((uint16_t)ST->next << 2);
    prow = p[0] | ((uint16_t)p[1] << 8);
    if (prow == 0xFFFFu || prow > row) break;
    if (ST->count < sizeof(ST->active)) ST->active[ST->count++] = ST->next;
    ++ST->next;
  }
  live = 0;
  for (i = 0; i < ST->count; ++i) {
    p = place + ((uint16_t)ST->active[i] << 2);
    prow = p[0] | ((uint16_t)p[1] << 8);
    col = p[2];
    flip = p[3] & 0x80u;
    st = stamps + ((p[3] & 0x7Fu) << 2);
    w = st[0];
    h = st[1];
    if (row >= prow + h) continue;      /* finished: drop it */
    ST->active[live++] = ST->active[i];
    off = st[2] | ((uint16_t)st[3] << 8);
    off += (uint16_t)(prow + h - 1u - row) * w;
    src = cells + (off << 1);
    if (flip) {
      src += (uint16_t)(w - 1u) << 1;
      for (x = 0; x < w; ++x, src -= 2) {
        dw_rowbuf[col + x] = src[0];
        dw_rowatt[col + x] = src[1] ^ 0x20u;
      }
    } else {
      for (x = 0; x < w; ++x, src += 2) {
        dw_rowbuf[col + x] = src[0];
        dw_rowatt[col + x] = src[1];
      }
    }
  }
  ST->count = live;
  write_row((uint8_t)(31u - (uint8_t)row) & 31u, now);
}

void dw_world_flat_row(uint8_t map_row) BANKED {
  VBK_REG = 1;
  fill_bkg_rect(0, map_row, 32, 1, flat_attr);
  VBK_REG = 0;
  fill_bkg_rect(0, map_row, 32, 1, flat_tile);
}

/* Boss tiles go to VRAM bank 1 at sector start, while the screen is black;
   flight scenery never uses those tiles. */
void dw_boss_tiles(uint8_t sector) BANKED {
  uint8_t b = (uint8_t)(sector - 1u) & 3u;
  VBK_REG = 1;
  set_bkg_data(0, boss_ntiles[b], boss_tiles[b]);
  VBK_REG = 0;
}

/* Arena rows: map rows 0-6 carry the boss in columns 0-11, every other row
   is flat sky. Rows are staged in work RAM (the scenery tables are done
   with by now) and copied by the VBlank DMA queue, three rows a frame. */
#define ARENA_BUF ((uint8_t *)0xD680)
void dw_arena_rows(uint8_t first, uint8_t count, uint8_t sector) BANKED {
  const uint8_t *map;
  uint8_t *t, *a, x, y;
  uint16_t dst;
  for (y = first; y != (uint8_t)(first + count); ++y) {
    if (y < DW_BOSS_H && sector) {
      t = ARENA_BUF + ((uint16_t)y << 6);
      map = boss_map[(uint8_t)(sector - 1u) & 3u] + (uint16_t)y * (DW_BOSS_W * 2u);
    } else {
      t = ARENA_BUF + (7u << 6);
      map = 0;
    }
    a = t + 32;
    for (x = 0; x < 32; ++x) {
      if (map && x < DW_BOSS_W) {
        t[x] = *map++;
        a[x] = *map++;
      } else {
        t[x] = flat_tile;
        a[x] = flat_attr;
      }
    }
    dst = 0x9800u + ((uint16_t)(y & 31u) << 5);
    dw_xfer(t, dst, 2, 0);
    dw_xfer(a, dst, 2, 1);
  }
}

void dw_pal_boss(uint8_t sector) BANKED {
  const uint16_t *src = boss_pal[(uint8_t)(sector - 1u) & 3u];
  uint8_t i;
  for (i = 0; i < 20; ++i) dw_pal[4 + i] = src[i];
}
