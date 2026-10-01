/* Shared state, hardware helpers and battery save.
   The launcher owns A000; Dotwing exclusively owns A200..A23F. */
#ifdef ARCADE
#pragma bank 30
#else
#pragma bank 5
#endif
#include "dotwing.h"

DotProfile dw_profile;
uint8_t dw_state, dw_keys, dw_pressed, dw_muted, dw_sector, dw_hp,
  dw_invincible, dw_power, dw_energy, dw_burst, dw_boss_hp, dw_boss_max,
  dw_boss_active, dw_combo, dw_victory, dw_focus, dw_menu_row, dw_dirty;
uint16_t dw_frame, dw_clock, dw_score, dw_best, dw_run_tokens, dw_seed,
  dw_stage_frame, dw_kills;
int16_t dw_px, dw_py, dw_boss_x, dw_boss_y;
DwEnemy dw_enemies[DW_ENEMIES];
DwBullet dw_shots[DW_SHOTS], dw_bullets[DW_BULLETS];
DwDrop dw_drops[DW_DROPS];

static uint8_t save_slot, save_generation;
static uint8_t digits[5];

void dw_hide(void) BANKED {
  uint8_t i;
  for (i = 0; i < 40; ++i) hide_sprite(i);
}

void dw_clear(uint8_t palette) BANKED {
  DISPLAY_OFF;
  HIDE_WIN;
  dw_hide();
  move_bkg(0, 0);
  move_win(7, 0);
  VBK_REG = 0;
  fill_bkg_rect(0, 0, 32, 32, 0);
  fill_win_rect(0, 0, 32, 32, 0);
  VBK_REG = 1;
  fill_bkg_rect(0, 0, 32, 32, palette);
  fill_win_rect(0, 0, 32, 32, palette);
  VBK_REG = 0;
}

void dw_label_buffer(uint8_t x, uint8_t y, uint8_t palette) BANKED {
  const char *s = dw_text;
  uint8_t start = x;
  uint8_t c;
  if (y >= 32 || x >= 20) return;
  while (*s && x < 20) {
    c = (uint8_t)*s++;
    if (c < 32 || c > 126) c = 32;
    set_bkg_tile_xy(x++, y, c - 31u);
  }
  if (x != start) {
    VBK_REG = 1;
    fill_bkg_rect(start, y, x - start, 1, palette);
    VBK_REG = 0;
  }
}

void dw_number(uint8_t x, uint8_t y, uint16_t n, uint8_t width,
               uint8_t palette) BANKED {
  uint8_t i;
  if (!width || x >= 20 || y >= 32) return;
  if (width > 5) width = 5;
  if (x + width > 20) width = 20 - x;
  i = width;
  while (i) {
    digits[--i] = 17u + n % 10u;
    n /= 10u;
  }
  VBK_REG = 0;
  set_bkg_tiles(x, y, width, 1, digits);
  VBK_REG = 1;
  fill_bkg_rect(x, y, width, 1, palette);
  VBK_REG = 0;
}

void dw_sprite(uint8_t id, uint8_t tile, int16_t x, int16_t y,
               uint8_t palette) BANKED {
  if (x < -7 || x > 159 || y < -15 || y > 143) {
    hide_sprite(id);
    return;
  }
  set_sprite_tile(id, tile);
  set_sprite_prop(id, palette);
  move_sprite(id, (uint8_t)(x + 8), (uint8_t)(y + 16));
}

void dw_actor(uint8_t id, uint8_t tile, int16_t x, int16_t y,
              uint8_t palette) BANKED {
  dw_sprite(id, tile, x, y, palette);
  dw_sprite(id + 1u, tile + 2u, x + 8, y, palette);
}

void dw_tone(uint16_t frequency, uint8_t envelope) BANKED {
  if (dw_muted) return;
  NR10_REG = 0;
  NR11_REG = 0x80;
  NR12_REG = envelope;
  NR13_REG = (uint8_t)frequency;
  NR14_REG = 0x80u | (uint8_t)(frequency >> 8);
}

void dw_noise(uint8_t pitch) BANKED {
  if (dw_muted) return;
  NR41_REG = 0x10;
  NR42_REG = 0x81;
  NR43_REG = pitch;
  NR44_REG = 0x80;
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
}

/* Fixed byte layout avoids compiler struct packing differences. Both slots
   have checksums, and the commit byte is written after the complete payload.
   A torn write leaves the preceding committed slot available. */
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
      p[13] > 3 || p[14] > 4) return 0;
  check = p[28] | (uint16_t)p[29] << 8;
  return check == save_sum(p);
}

void dw_load(void) BANKED {
  uint8_t raw[64];
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
}

void dw_save(void) BANKED {
  uint8_t raw[32];
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
