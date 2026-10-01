/* Scenery for sectors 1 and 2: tiles go to VRAM, layout tables to work RAM. */
#ifndef ARCADE
#pragma bank 1
#endif
#include "dotwing.h"
#include <string.h>
#include "terraina_data.h"

static void load(const uint8_t *tiles, uint16_t n, const uint8_t *base, const uint8_t *stamps,
                 uint8_t ns, const uint8_t *place, uint16_t np, const uint8_t *cells, uint16_t nc,
                 const palette_color_t *pal) {
  uint8_t i;
  VBK_REG = 0;
  set_bkg_data(128, n > 128 ? 128 : (uint8_t)n, tiles);
  if (n > 128) {
    VBK_REG = 1;
    set_bkg_data(128, (uint8_t)(n - 128u), tiles + 2048u);
    VBK_REG = 0;
  }
  memcpy(DW_TB_BASE, base, 64);
  memcpy(DW_TB_STAMPS, stamps, ns);
  memcpy(DW_TB_PLACE, place, np);
  memcpy(DW_TB_CELLS, cells, nc);
  for (i = 0; i < 24; ++i) dw_pal[i] = pal[i];
}

void dw_terrain_a(uint8_t s) BANKED {
  if (s == 0)
    load(s0_tiles, S0_NTILES, s0_base, s0_stamps, sizeof(s0_stamps), s0_place,
         sizeof(s0_place), s0_cells, sizeof(s0_cells), s0_pal);
  else
    load(s1_tiles, S1_NTILES, s1_base, s1_stamps, sizeof(s1_stamps), s1_place,
         sizeof(s1_place), s1_cells, sizeof(s1_cells), s1_pal);
}
