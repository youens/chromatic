/* Scenery for sectors 3 and 4: tiles go to VRAM, layout tables to work RAM. */
#ifndef ARCADE
#pragma bank 2
#endif
#include "dotwing.h"
#include <string.h>
#include "terrainb_data.h"

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

void dw_terrain_b(uint8_t s) BANKED {
  if (s == 2)
    load(s2_tiles, S2_NTILES, s2_base, s2_stamps, sizeof(s2_stamps), s2_place,
         sizeof(s2_place), s2_cells, sizeof(s2_cells), s2_pal);
  else
    load(s3_tiles, S3_NTILES, s3_base, s3_stamps, sizeof(s3_stamps), s3_place,
         sizeof(s3_place), s3_cells, sizeof(s3_cells), s3_pal);
}
