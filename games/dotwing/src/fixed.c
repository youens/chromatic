/* Shared state, frame sync, palette fades, map helpers and battery save.
   The launcher owns SRAM A000; Dotwing exclusively owns A200..A23F. */
#ifndef ARCADE
#pragma bank 5
#endif
#include "dotwing.h"

DotProfile dw_profile;
uint8_t dw_state, dw_keys, dw_pressed, dw_muted, dw_sector, dw_hp,
  dw_invincible, dw_power, dw_energy, dw_burst, dw_boss_active, dw_combo,
  dw_victory, dw_focus, dw_menu_row, dw_dirty, dw_fade, dw_pal_dirty,
  dw_flash_mask, dw_flash_level, dw_scx, dw_scy, dw_shake, dw_allies,
  dw_new_best, dw_dim;
uint16_t dw_frame, dw_clock, dw_score, dw_best, dw_run_tokens, dw_seed,
  dw_stage_frame, dw_kills, dw_boss_hp, dw_boss_max;
int16_t dw_px, dw_py, dw_boss_x, dw_boss_y;
uint8_t *dw_oam;
uint8_t dw_oam_n;

uint8_t __at(0xD000) dw_rowbuf[32];
uint8_t __at(0xD020) dw_rowatt[32];
uint8_t __at(0xD040) dw_scratch[64];
uint8_t __at(0xD080) dw_hudbuf[64];
uint8_t __at(0xD0C0) dw_hudatt[64];
uint8_t __at(0xD100) dw_oam_b[160];
DwFoe __at(0xD1A0) dw_foes[DW_FOES];
DwShot __at(0xD220) dw_shots[DW_SHOTS];
DwShot __at(0xD2A0) dw_bullets[DW_BULLETS];
DwDrop __at(0xD390) dw_drops[DW_DROPS];
DwFx __at(0xD3C0) dw_fx[DW_FXS];
uint16_t __at(0xD400) dw_pal[64];
uint8_t __at(0xD480) dw_trail[64];
uint8_t __at(0xD500) dw_stream[64];
static uint8_t __at(0xD600) palbuf[128];

/* Colour channel scaled by level/8, and lifted towards white by level/8. */
static const uint8_t fade_tab[9][32] = {
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3},
  {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7},
  {0,0,0,1,1,1,2,2,3,3,3,4,4,4,5,5,6,6,6,7,7,7,8,8,9,9,9,10,10,10,11,11},
  {0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15},
  {0,0,1,1,2,3,3,4,5,5,6,6,7,8,8,9,10,10,11,11,12,13,13,14,15,15,16,16,17,18,18,19},
  {0,0,1,2,3,3,4,5,6,6,7,8,9,9,10,11,12,12,13,14,15,15,16,17,18,18,19,20,21,21,22,23},
  {0,0,1,2,3,4,5,6,7,7,8,9,10,11,12,13,14,14,15,16,17,18,19,20,21,21,22,23,24,25,26,27},
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31}
};
static const uint8_t lift_tab[9][32] = {
  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31},
  {3,4,5,6,7,8,9,10,10,11,12,13,14,15,16,17,17,18,19,20,21,22,23,24,24,25,26,27,28,29,30,31},
  {7,8,9,10,10,11,12,13,13,14,15,16,16,17,18,19,19,20,21,22,22,23,24,25,25,26,27,28,28,29,30,31},
  {11,12,12,13,14,14,15,16,16,17,17,18,19,19,20,21,21,22,22,23,24,24,25,26,26,27,27,28,29,29,30,31},
  {15,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31},
  {19,19,20,20,20,21,21,22,22,22,23,23,23,24,24,25,25,25,26,26,26,27,27,28,28,28,29,29,29,30,30,31},
  {23,23,23,24,24,24,24,25,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,30,30,30,30,31},
  {27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,29,29,29,29,29,29,29,29,30,30,30,30,30,30,30,30,31},
  {31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31}
};

static uint8_t save_slot, save_generation;
static const int8_t shake_x[8] = {0, 2, -2, 1, -1, 2, -1, 0};
static const int8_t shake_y[8] = {0, -1, 2, -2, 1, 0, 2, -1};

/* Copy the prepared palette buffer into CGB palette RAM. Unrolled so all
   128 bytes land well inside VBlank, even after the sound handler. */
static void pal_upload(void) __naked {
  __asm
    ld hl, #0xD600
    ld a, #0x80
    ldh (0x68), a
    ld c, #0x69
    ld b, #8
  1$:
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    dec b
    jr nz, 1$
    ld a, #0x80
    ldh (0x6A), a
    ld c, #0x6B
    ld b, #8
  2$:
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    ld a, (hl+)
    ldh (c), a
    dec b
    jr nz, 2$
    ret
  __endasm;
}

/* Start building this frame's sprites in the buffer DMA is not reading. */
void dw_oam_begin(void) BANKED {
  dw_oam = _shadow_OAM_base == 0xC0 ? dw_oam_b : (uint8_t *)0xC000;
  dw_oam_n = 0;
}

/* Hide the sprites this frame did not use and hand the buffer to OAM DMA. */
void dw_oam_flip(void) BANKED {
  uint8_t *p = dw_oam + (dw_oam_n << 2);
  uint8_t n = 40 - dw_oam_n;
  while (n--) {
    *p = 0;
    p += 4;
  }
  _shadow_OAM_base = (uint8_t)((uint16_t)dw_oam >> 8);
}

/* Finish the frame, wait for VBlank, then apply scroll and palette changes
   while the LCD is not drawing. */
/* Frames where logic overran VBlank: emulator tests watch this count. */
uint16_t dw_missed;
static uint8_t last_vbl;

/* VBlank transfer queue: CGB general-purpose DMA copies 16-byte blocks from
   work RAM into VRAM in microseconds, so map rows and the HUD never wait on
   HBlank one byte at a time. Sources and targets are 16-byte aligned. */
typedef struct { uint16_t src, dst; uint8_t blocks, bank; } Xfer;
static Xfer xq[6];
static uint8_t xn;

void dw_xfer(const uint8_t *src, uint16_t dst, uint8_t blocks, uint8_t bank) BANKED {
  Xfer *x;
  if (xn >= 6) return;
  x = &xq[xn++];
  x->src = (uint16_t)src;
  x->dst = dst;
  x->blocks = blocks;
  x->bank = bank;
}

static void xfer_run(void) {
  uint8_t i;
  Xfer *x = xq;
  for (i = 0; i < xn; ++i, ++x) {
    VBK_REG = x->bank;
    HDMA1_REG = (uint8_t)(x->src >> 8);
    HDMA2_REG = (uint8_t)x->src;
    HDMA3_REG = (uint8_t)(x->dst >> 8);
    HDMA4_REG = (uint8_t)x->dst;
    HDMA5_REG = x->blocks - 1u;
  }
  VBK_REG = 0;
  xn = 0;
}

void dw_sync(void) BANKED {
  dw_oam_flip();
  vsync();
  if ((uint8_t)((uint8_t)sys_time - last_vbl) > 1) ++dw_missed;
  last_vbl = (uint8_t)sys_time;
  if (dw_pal_dirty) {
    pal_upload();
    dw_pal_dirty = 0;
  }
  if (xn) xfer_run();
  if (dw_shake) {
    --dw_shake;
    SCX_REG = dw_scx + shake_x[dw_shake & 7];
    SCY_REG = dw_scy + shake_y[dw_shake & 7];
  } else {
    SCX_REG = dw_scx;
    SCY_REG = dw_scy;
  }
}

/* Faded or brightened copy of target colours first..first+count-1; the
   whole buffer is uploaded at the next VBlank. */
void dw_pal_commit_part(uint8_t first, uint8_t count) BANKED {
  uint8_t i, r, g, b, end = first + count;
  uint16_t c;
  uint8_t *out = palbuf + (first << 1);
  const uint8_t *fade = fade_tab[dw_fade > 8 ? 8 : dw_fade];
  const uint8_t *dim = fade_tab[dw_dim ? 4 : (dw_fade > 8 ? 8 : dw_fade)];
  const uint8_t *lift = lift_tab[dw_flash_level > 8 ? 8 : dw_flash_level];
  uint8_t mask = dw_flash_mask;
  for (i = first; i != end; ++i) {
    c = dw_pal[i];
    if (i < 24 || i >= 32) {
      r = dim[c & 31];
      g = dim[(c >> 5) & 31];
      b = dim[(c >> 10) & 31];
      if (dw_dim && dw_fade < 8) {
        r = fade[r];
        g = fade[g];
        b = fade[b];
      }
    } else {
      r = fade[c & 31];
      g = fade[(c >> 5) & 31];
      b = fade[(c >> 10) & 31];
    }
    if (i < 32 && (i & 3u) && (mask & (1 << (i >> 2)))) {
      r = lift[r];
      g = lift[g];
      b = lift[b];
    }
    c = r | ((uint16_t)g << 5) | ((uint16_t)b << 10);
    *out++ = (uint8_t)c;
    *out++ = (uint8_t)(c >> 8);
  }
  dw_pal_dirty = 1;
}

void dw_pal_commit(void) BANKED {
  dw_pal_commit_part(0, 64);
}

/* Upload palettes at once: only while the LCD is off or in VBlank. */
void dw_pal_now(void) BANKED {
  dw_pal_commit();
  pal_upload();
  dw_pal_dirty = 0;
}

/* Fade the whole display to `level` (0 black, 8 full) one step a frame.
   The LCD stays on, sprites hold still, and nothing ever flashes white. */
void dw_fade_to(uint8_t level) BANKED {
  while (dw_fade != level) {
    if (dw_fade < level) ++dw_fade;
    else --dw_fade;
    dw_pal_commit();
    vsync();
    pal_upload();
    dw_pal_dirty = 0;
    if (xn) xfer_run();
  }
}

void dw_clear_map(uint8_t tile, uint8_t attr) BANKED {
  VBK_REG = 1;
  fill_bkg_rect(0, 0, 32, 32, attr);
  VBK_REG = 0;
  fill_bkg_rect(0, 0, 32, 32, tile);
}

void dw_clear_win(void) BANKED {
  VBK_REG = 1;
  fill_win_rect(0, 0, 20, 18, PAL_HUD);
  VBK_REG = 0;
  fill_win_rect(0, 0, 20, 18, 0);
}

void dw_attr(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t attr) BANKED {
  VBK_REG = 1;
  fill_bkg_rect(x, y, w, h, attr);
  VBK_REG = 0;
}

/* Width with bit 7 set pads with spaces instead of leading zeros. */
static uint8_t digits(uint16_t n, uint8_t width) {
  uint8_t i, pad = width & 0x80u;
  width &= 0x7Fu;
  i = width;
  while (i) {
    dw_scratch[--i] = '0' + (uint8_t)(n % 10u);
    n /= 10u;
    if (pad && !n) break;
  }
  while (i) dw_scratch[--i] = ' ';
  return width;
}

void dw_number(uint8_t x, uint8_t y, uint16_t n, uint8_t width, uint8_t attr) BANKED {
  width = digits(n, width);
  set_bkg_tiles(x, y, width, 1, dw_scratch);
  dw_attr(x, y, width, 1, attr);
}

void dw_wnumber(uint8_t x, uint8_t y, uint16_t n, uint8_t width, uint8_t attr) BANKED {
  width = digits(n, width);
  set_win_tiles(x, y, width, 1, dw_scratch);
  VBK_REG = 1;
  fill_win_rect(x, y, width, 1, attr);
  VBK_REG = 0;
}

uint16_t dw_random(void) BANKED {
  if (!dw_seed) dw_seed = 0xD071;
  dw_seed ^= dw_seed << 7;
  dw_seed ^= dw_seed >> 9;
  dw_seed ^= dw_seed << 8;
  return dw_seed;
}

void dw_points(uint16_t n) BANKED {
  if (dw_score > 65535u - n) dw_score = 65535u;
  else dw_score += n;
  dw_dirty |= 1;
}

/* Fixed byte layout avoids compiler struct packing differences. Both slots
   have checksums, and the commit byte is written after the complete payload.
   A torn write leaves the preceding committed slot available. Byte 19 (the
   ally upgrade) was zero in earlier releases, so their saves still load. */
static uint16_t save_sum(const uint8_t *p) {
  uint8_t i;
  uint16_t sum = 0xD071;
  for (i = 0; i < 28; ++i)
    sum = ((sum << 1) | (sum >> 15)) ^ p[i];
  return sum;
}

static uint8_t save_valid(const uint8_t *p) {
  uint16_t check;
  if (p[31] != 0xA5 || p[0] != 'D' || p[1] != 'O' ||
      p[2] != 'T' || p[3] != 'W' || p[4] != 1) return 0;
  if (p[6] >= DW_SHAPES || p[7] >= DW_COLORS || p[8] >= DW_FACES ||
      p[9] >= DW_GEARS || p[10] > 1 || p[11] > 3 || p[12] > 3 ||
      p[13] > 3 || p[14] > 4 || p[19] > 3) return 0;
  check = p[28] | (uint16_t)p[29] << 8;
  return check == save_sum(p);
}

void dw_load(void) BANKED {
  uint8_t *raw = dw_scratch;
  uint8_t a, b, use;
  const uint8_t *p;
  uint8_t i;
  for (i = 0; i < sizeof(dw_profile); ++i) ((uint8_t *)&dw_profile)[i] = 0;
  dw_best = 0;
  save_slot = 1;
  save_generation = 255;
  /* SRAM banking does not modify either MBC5 ROM-bank register. */
  ENABLE_RAM;
  SWITCH_RAM(0);
  for (i = 0; i < 64; ++i) raw[i] = ((volatile uint8_t *)0xA200)[i];
  DISABLE_RAM;
  a = save_valid(raw);
  b = save_valid(raw + 32);
  if (!a && !b) return;
  use = b && (!a || (uint8_t)(raw[37] - raw[5]) < 128u);
  p = raw + (use ? 32 : 0);
  save_slot = use;
  save_generation = p[5];
  dw_profile.shape = p[6];
  dw_profile.color = p[7];
  dw_profile.face = p[8];
  dw_profile.gear = p[9];
  dw_profile.configured = p[10];
  dw_profile.weapon = p[11];
  dw_profile.shield = p[12];
  dw_profile.reactor = p[13];
  dw_profile.cleared = p[14];
  dw_profile.tokens = p[15] | (uint16_t)p[16] << 8;
  dw_best = p[17] | (uint16_t)p[18] << 8;
  dw_profile.ally = p[19];
}

void dw_save(void) BANKED {
  uint8_t *raw = dw_scratch;
  uint8_t i;
  uint16_t check;
  volatile uint8_t *target;
  for (i = 0; i < 32; ++i) raw[i] = 0;
  raw[0] = 'D'; raw[1] = 'O'; raw[2] = 'T'; raw[3] = 'W';
  raw[4] = 1;
  raw[5] = ++save_generation;
  raw[6] = dw_profile.shape;
  raw[7] = dw_profile.color;
  raw[8] = dw_profile.face;
  raw[9] = dw_profile.gear;
  raw[10] = dw_profile.configured;
  raw[11] = dw_profile.weapon;
  raw[12] = dw_profile.shield;
  raw[13] = dw_profile.reactor;
  raw[14] = dw_profile.cleared;
  raw[15] = (uint8_t)dw_profile.tokens;
  raw[16] = (uint8_t)(dw_profile.tokens >> 8);
  raw[17] = (uint8_t)dw_best;
  raw[18] = (uint8_t)(dw_best >> 8);
  raw[19] = dw_profile.ally;
  check = save_sum(raw);
  raw[28] = (uint8_t)check;
  raw[29] = (uint8_t)(check >> 8);
  save_slot ^= 1u;
  target = (volatile uint8_t *)(save_slot ? 0xA220 : 0xA200);
  ENABLE_RAM;
  SWITCH_RAM(0);
  target[31] = 0;
  for (i = 0; i < 31; ++i) target[i] = raw[i];
  target[31] = 0xA5;
  DISABLE_RAM;
}
