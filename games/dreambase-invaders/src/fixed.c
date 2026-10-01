/* Fixed-bank code: VBlank and scanline handlers, map and palette transfer,
   and the drawing helpers that take pointers into the caller's bank. */
#include "dreambase.h"
#include "colors.h"
#include <string.h>

uint8_t __at(0xD280) pr_on[MAXP];
int16_t __at(0xD28C) pr_x[MAXP];
int16_t __at(0xD2A4) pr_y[MAXP];
int16_t __at(0xD2BC) pr_vx[MAXP];
int16_t __at(0xD2D4) pr_vy[MAXP];
uint8_t __at(0xD2EC) pr_bx[MAXP];
uint8_t __at(0xD2F8) pr_ph[MAXP];
uint8_t __at(0xD304) pr_logo[MAXP];
uint8_t __at(0xD310) pk_life[MAXK];
int16_t __at(0xD320) pk_x[MAXK];
int16_t __at(0xD340) pk_y[MAXK];
int16_t __at(0xD360) pk_vx[MAXK];
int16_t __at(0xD380) pk_vy[MAXK];
uint8_t __at(0xD3A0) pk_tile[MAXK];
uint8_t __at(0xD3B0) pk_pal[MAXK];

uint8_t db_keys, db_pressed, db_home, db_muted, db_oam, db_fade;
uint8_t db_scx, db_scy, db_wx, db_wy, db_map_hi, db_rmode;
uint8_t db_wave_t, db_copper_t, db_band, db_shine;
uint16_t db_face;
volatile uint16_t db_pal_dirty;
uint8_t db_row_dirty[MAP_ROWS];
uint16_t db_frame;
static uint8_t installed, shine_line;

/* ------------------------------------------------------------ transfers */
/* Copy dirty shadow rows (tiles and attributes) with general-purpose DMA. */
static void flush_rows(uint8_t budget) {
  uint8_t r, hi, lo, src;
  for (r = 0; r < MAP_ROWS && budget; r++) {
    if (!db_row_dirty[r])
      continue;
    db_row_dirty[r] = 0;
    budget--;
    src = 0xD0 + (r >> 3);
    lo = (uint8_t)(r << 5);
    hi = (db_map_hi & 0x1F) + (r >> 3);
    VBK_REG = 0;
    HDMA1_REG = src;
    HDMA2_REG = lo;
    HDMA3_REG = hi;
    HDMA4_REG = lo;
    HDMA5_REG = 1;
    VBK_REG = 1;
    HDMA1_REG = src + 4;
    HDMA2_REG = lo;
    HDMA3_REG = hi;
    HDMA4_REG = lo;
    HDMA5_REG = 1;
  }
}
static void upload(void) {
  uint16_t m = db_pal_dirty;
  const uint8_t *p = (const uint8_t *)PAL_LIVE;
  uint8_t i, j;
  db_pal_dirty = 0;
  for (i = 0; m; i++, m >>= 1, p += 8) {
    if (!(m & 1))
      continue;
    if (i < 8) {
      BCPS_REG = 0x80 | (i << 3);
      for (j = 0; j < 8; j++)
        BCPD_REG = p[j];
    } else {
      OCPS_REG = 0x80 | ((i - 8) << 3);
      for (j = 0; j < 8; j++)
        OCPD_REG = p[j];
    }
  }
}
static void vbl(void) {
  uint8_t vbk = VBK_REG;
  upload();
  flush_rows(6);
  VBK_REG = vbk;
  SCX_REG = db_scx;
  SCY_REG = db_scy;
  WX_REG = db_wx;
  WY_REG = db_wy;
  if (db_band == 0xF1) {
    /* The frame ended inside the shine: restore its colours now. */
    BCPS_REG = 0x86;
    BCPD_REG = (uint8_t)PAL_LIVE[3];
    BCPD_REG = (uint8_t)(PAL_LIVE[3] >> 8);
    BCPS_REG = 0x8E;
    BCPD_REG = (uint8_t)PAL_LIVE[7];
    BCPD_REG = (uint8_t)(PAL_LIVE[7] >> 8);
  }
  shine_line = db_shine;
  STAT_REG = 0x40;
  if (db_rmode != 1) {
    db_band = 0xFF;
    LYC_REG = 255;
  } else if (shine_line) {
    db_band = 0xF0;
    LYC_REG = shine_line - 1;
  } else {
    db_band = 0xF2;
    LYC_REG = 31;
  }
}
/* Title raster. Two LY=LYC events switch a four-line shine on and off as
   it sweeps down the wordmark (white letters turn green, the green mark
   turns white). At line 31 the handler switches to the HBlank interrupt:
   each HBlank of the INVADERS band sets the next line's wobble and copper
   colour straight away. Re-arming LYC for the very next line would race
   the interrupt epilogue, which acknowledges the STAT flag on its way out. */
static void lcd(void) {
  uint8_t l = db_band, x;
  uint16_t c;
  if (l == 0xF0 || l == 0xF1) {
    while (STAT_REG & 3)
      ;
    BCPS_REG = 0x86;
    c = l == 0xF0 ? RGB(2, 29, 14) : PAL_LIVE[3];
    BCPD_REG = (uint8_t)c;
    BCPD_REG = (uint8_t)(c >> 8);
    BCPS_REG = 0x8E;
    c = l == 0xF0 ? RGB(31, 31, 31) : PAL_LIVE[7];
    BCPD_REG = (uint8_t)c;
    BCPD_REG = (uint8_t)(c >> 8);
    if (l == 0xF0) {
      db_band = 0xF1;
      LYC_REG = shine_line + 3;
    } else {
      db_band = 0xF2;
      LYC_REG = 31;
    }
    return;
  }
  if (l == 0xF2) {
    db_band = 0;
    STAT_REG = 0x08;
    return;
  }
  if (l < 24) {
    x = db_scx + WAVE[(uint8_t)(l + db_wave_t) & 31];
    c = copper_cols[(uint8_t)((l >> 1) + db_copper_t) & 31];
    db_band = l + 1;
  } else {
    x = db_scx;
    c = db_face;
    STAT_REG = 0x40;
    LYC_REG = 255;
    db_band = 0xFF;
  }
  SCX_REG = x;
  BCPS_REG = 0x92;
  BCPD_REG = (uint8_t)c;
  BCPD_REG = (uint8_t)(c >> 8);
}
void db_raster_on(void) {
  if (installed)
    return;
  disable_interrupts();
  add_VBL(vbl);
  add_LCD(lcd);
  LYC_REG = 255;
  STAT_REG = 0x40; /* LY=LYC interrupt */
  installed = 1;
  enable_interrupts();
  set_interrupts(VBL_IFLAG | LCD_IFLAG);
}
void db_raster_off(void) {
  if (!installed)
    return;
  set_interrupts(VBL_IFLAG);
  disable_interrupts();
  remove_LCD(lcd);
  remove_VBL(vbl);
  STAT_REG = 0;
  LYC_REG = 255;
  SCX_REG = SCY_REG = 0;
  db_rmode = 0;
  installed = 0;
  enable_interrupts();
}
void db_flush_all(void) {
  uint8_t r, vbk = VBK_REG;
  for (r = 0; r < MAP_ROWS; r++)
    db_row_dirty[r] = 1;
  flush_rows(MAP_ROWS);
  upload();
  VBK_REG = vbk;
}

/* --------------------------------------------------------------- maps */
uint8_t db_strlen(const char *s) {
  uint8_t n = 0;
  while (s[n])
    n++;
  return n;
}
void db_text(uint8_t x, uint8_t y, const char *s, uint8_t a) {
  uint16_t i = ((uint16_t)y << 5) + x;
  uint8_t c;
  while ((c = *s++) && x++ < 20) {
    MAP[i] = c == ' ' ? 0 : c - 31;
    ATT[i++] = a;
  }
  db_row_dirty[y] = 1;
}
void db_center(uint8_t y, const char *s, uint8_t a) {
  db_text((20 - db_strlen(s)) >> 1, y, s, a);
}
/* Double-height letters from VRAM bank 1; see TALL_CHARS in assets.py. */
static uint8_t tall_index(uint8_t c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 1;
  if (c >= '0' && c <= '9')
    return c - '0' + 27;
  return c == '!' ? 37 : c == '.' ? 38 : c == ':' ? 39 : c == '+' ? 40 : 0;
}
void db_tall(uint8_t x, uint8_t y, const char *s, uint8_t a) {
  uint8_t c, t;
  while ((c = *s++) && x < 20) {
    t = tall_index(c) << 1;
    db_put(x, y, t, a | 0x08);
    db_put(x++, y + 1, t + 1, a | 0x08);
  }
}
/* ----------------------------------------------------------- palettes */
/* c * f / 8 for each channel, by shifts (f is 0-8). */
static uint8_t part(uint8_t v, uint8_t f) {
  uint8_t s = 0;
  if (f & 1)
    s += v;
  if (f & 2)
    s += v << 1;
  if (f & 4)
    s += v << 2;
  return s >> 3;
}
uint16_t db_scale(uint16_t c, uint8_t f) {
  if (!f)
    return 0;
  return part(c & 31, f) | ((uint16_t)part((c >> 5) & 31, f) << 5) |
         ((uint16_t)part((c >> 10) & 31, f) << 10);
}
void db_pal(uint8_t slot, const uint16_t *c) {
  uint8_t i, k = slot << 2;
  for (i = 0; i < 4; i++) {
    PAL_TARGET[k + i] = c[i];
    PAL_LIVE[k + i] = db_fade >= 8 ? c[i] : db_scale(c[i], db_fade);
  }
  db_pal_dirty |= (uint16_t)1 << slot;
}
