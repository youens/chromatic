/* Entry point, frame loop, fades and the sound engine. */
#include "dreambase.h"
#include <string.h>

uint16_t db_best, db_score;
uint8_t db_mode, db_diff, db_level, db_won;
static uint8_t previous;

/* ---------------------------------------------------------------- notes */
/* Square-channel frequency registers for MIDI notes 36 (C2) to 108 (C8).
   The wave channel sounds an octave lower, so bass reads 12 notes up. */
static const uint16_t note_x[73] = {
    44, 157, 263, 363, 457, 547, 631, 711, 786, 856, 923, 986,
    1046, 1102, 1155, 1205, 1253, 1297, 1339, 1379, 1417, 1452, 1486, 1517,
    1547, 1575, 1602, 1627, 1650, 1673, 1694, 1714, 1732, 1750, 1767, 1783,
    1798, 1812, 1825, 1837, 1849, 1860, 1871, 1881, 1890, 1899, 1907, 1915,
    1923, 1930, 1936, 1943, 1949, 1954, 1959, 1964, 1969, 1974, 1978, 1982,
    1985, 1989, 1992, 1995, 1998, 2001, 2004, 2006, 2009, 2011, 2013, 2015,
    2017,
};
#define NOTE(n) note_x[(n) - 36]

/* Songs are (note, frames) pairs; note 0 rests and 0xFF ends the part. */
#define E 14 /* one eighth at about 128 beats per minute */
static const uint8_t title_lead[] = {
    76, E, 81, E, 84, 2 * E, 83, E, 81, E, 76, 2 * E,
    77, E, 81, E, 84, 2 * E, 86, E, 84, E, 81, 2 * E,
    79, E, 84, E, 88, 2 * E, 86, E, 84, E, 79, 2 * E,
    79, E, 83, E, 86, 2 * E, 88, E, 86, E, 83, 2 * E,
    81, E, 84, E, 88, 2 * E, 86, E, 84, E, 81, 2 * E,
    81, E, 84, E, 89, 2 * E, 88, E, 84, E, 81, 2 * E,
    79, E, 84, E, 88, E, 91, E, 88, 2 * E, 84, 2 * E,
    86, E, 83, E, 79, E, 83, E, 86, 4 * E, 0xFF};
static const uint8_t title_bass[] = {
    45, E, 57, E, 45, E, 57, E, 45, E, 57, E, 45, E, 57, E,
    41, E, 53, E, 41, E, 53, E, 41, E, 53, E, 41, E, 53, E,
    48, E, 60, E, 48, E, 60, E, 48, E, 60, E, 48, E, 60, E,
    43, E, 55, E, 43, E, 55, E, 43, E, 55, E, 43, E, 55, E, 0xFF};
/* Drums: 1 kick, 2 snare, 3 hat. */
static const uint8_t title_drums[] = {
    1, E, 3, E, 2, E, 3, E, 1, E, 3, E, 2, E, 3, E, 0xFF};
static const uint8_t logo_lead[] = {72, 4, 76, 4, 79, 4, 84, 4, 88, 40, 0xFF};
static const uint8_t logo_bass[] = {48, 16, 60, 40, 0xFF};
static const uint8_t start_lead[] = {83, 5, 88, 18, 0, 6, 67, 5, 72, 5, 76, 5, 79, 5, 84, 22, 0xFF};
static const uint8_t level_lead[] = {72, 5, 76, 5, 79, 5, 84, 5, 88, 5, 91, 5, 96, 32, 0xFF};
static const uint8_t level_bass[] = {48, 15, 55, 15, 60, 32, 0xFF};
static const uint8_t over_lead[] = {72, 12, 70, 12, 68, 12, 65, 12, 63, 12, 60, 30, 0xFF};
static const uint8_t over_bass[] = {0, 72, 36, 50, 0xFF};
static const uint8_t oneup_lead[] = {88, 6, 91, 6, 100, 6, 96, 6, 98, 6, 103, 10, 0xFF};
static const uint8_t credits_lead[] = {
    81, E, 84, E, 88, 2 * E, 86, E, 84, E, 81, 2 * E,
    77, E, 81, E, 84, 2 * E, 86, E, 88, E, 89, 2 * E,
    91, E, 88, E, 84, 2 * E, 86, E, 88, E, 91, 2 * E,
    93, 2 * E, 91, E, 88, E, 86, 4 * E, 0xFF};

typedef struct {
  const uint8_t *start, *pos;
  uint8_t wait, loop;
} Voice;
static Voice lead, bass, drums;
static uint8_t sfx_wait, sfx_step, sfx_id, sfx_arg, noise_wait;

static void voice(Voice *v, const uint8_t *data, uint8_t loop) {
  v->start = v->pos = data;
  v->wait = 0;
  v->loop = loop;
}
static void play_lead(uint8_t n) {
  if (!n) {
    NR22_REG = 0;
    NR24_REG = 0x80;
    return;
  }
  NR21_REG = 0x80;
  NR22_REG = 0x93;
  NR23_REG = (uint8_t)NOTE(n);
  NR24_REG = 0x80 | (NOTE(n) >> 8);
}
static void play_bass(uint8_t n) {
  if (!n) {
    NR32_REG = 0;
    return;
  }
  NR30_REG = 0x80;
  NR32_REG = 0x20;
  NR33_REG = (uint8_t)NOTE(n + 12);
  NR34_REG = 0x80 | (NOTE(n + 12) >> 8);
}
static void play_drum(uint8_t d) {
  if (!d || noise_wait)
    return;
  NR41_REG = 0;
  NR42_REG = d == 1 ? 0xA1 : d == 2 ? 0x82 : 0x31;
  NR43_REG = d == 1 ? 0x62 : d == 2 ? 0x34 : 0x10;
  NR44_REG = 0x80;
}
static void tick_voice(Voice *v, uint8_t kind) {
  uint8_t n;
  if (!v->pos)
    return;
  if (v->wait) {
    if (--v->wait == 2 && kind == 1)
      NR32_REG = 0; /* detach bass notes */
    return;
  }
  n = *v->pos;
  if (n == 0xFF) {
    if (!v->loop) {
      v->pos = 0;
      return;
    }
    v->pos = v->start;
    n = *v->pos;
  }
  v->wait = v->pos[1] - 1;
  v->pos += 2;
  if (kind == 0)
    play_lead(n);
  else if (kind == 1)
    play_bass(n);
  else
    play_drum(n);
}
void music(uint8_t song) DB_BANKED {
  lead.pos = bass.pos = drums.pos = 0;
  NR22_REG = 0;
  NR24_REG = 0x80;
  NR32_REG = 0;
  if (song == SONG_TITLE) {
    voice(&lead, title_lead, 1);
    voice(&bass, title_bass, 1);
    voice(&drums, title_drums, 1);
  } else if (song == SONG_LOGO) {
    voice(&lead, logo_lead, 0);
    voice(&bass, logo_bass, 0);
  } else if (song == SONG_START) {
    voice(&lead, start_lead, 0);
  } else if (song == SONG_LEVEL) {
    voice(&lead, level_lead, 0);
    voice(&bass, level_bass, 0);
  } else if (song == SONG_OVER) {
    voice(&lead, over_lead, 0);
    voice(&bass, over_bass, 0);
  } else if (song == SONG_ONEUP) {
    voice(&lead, oneup_lead, 0);
  } else if (song == SONG_CREDITS) {
    voice(&lead, credits_lead, 1);
    voice(&bass, title_bass, 1);
    voice(&drums, title_drums, 1);
  }
}
/* The gameplay heartbeat: one short bass note on the wave channel. */
void beat(uint8_t n) DB_BANKED {
  if (bass.pos)
    return;
  play_bass(n);
}

/* ------------------------------------------------------------------ sfx */
/* Channel 1 effects as short sequences of (note, frames, envelope, sweep). */
static const uint8_t fx_blip[] = {81, 4, 0x81, 0, 0};
static const uint8_t fx_eat[] = {81, 4, 0xA1, 0x1B, 74, 4, 0x91, 0x13, 0};
static const uint8_t fx_miss[] = {46, 14, 0xA2, 0x2E, 0};
static const uint8_t fx_lost[] = {64, 30, 0xF3, 0x77, 52, 20, 0xC3, 0x77, 0};
static const uint8_t fx_tier[] = {84, 4, 0xB1, 0, 91, 5, 0xB1, 0, 96, 8, 0xB2, 0, 0};
static const uint8_t fx_select[] = {76, 3, 0x91, 0, 83, 6, 0x91, 0, 0};
static const uint8_t *const effects[] = {fx_blip, fx_eat, fx_miss, fx_lost, fx_tier, 0, 0, fx_select};

static void tick_sfx(void) {
  const uint8_t *p;
  uint8_t n;
  if (noise_wait)
    noise_wait--;
  if (sfx_wait) {
    sfx_wait--;
    return;
  }
  if (!sfx_id)
    return;
  p = effects[sfx_id - 1] + sfx_step * 4;
  if (!p[0]) {
    sfx_id = 0;
    NR12_REG = 0;
    NR14_REG = 0x80;
    return;
  }
  n = p[0] + sfx_arg;
  NR10_REG = p[3];
  NR11_REG = 0x80;
  NR12_REG = p[2];
  NR13_REG = (uint8_t)NOTE(n);
  NR14_REG = 0x80 | (NOTE(n) >> 8);
  sfx_wait = p[1] - 1;
  sfx_step++;
}
void sfx(uint8_t id, uint8_t arg) DB_BANKED {
  if (id == SFX_DASH || id == SFX_FIRE || id == SFX_LOST) {
    NR41_REG = 0;
    NR42_REG = id == SFX_DASH ? 0x61 : id == SFX_FIRE ? 0x21 : 0xF4;
    NR43_REG = id == SFX_DASH ? 0x24 : id == SFX_FIRE ? 0x55 : 0x67;
    NR44_REG = 0x80;
    noise_wait = id == SFX_LOST ? 40 : 8;
    if (id != SFX_LOST)
      return;
  }
  sfx_id = id + 1;
  sfx_arg = arg;
  sfx_step = 0;
  sfx_wait = 0;
  tick_sfx();
}


/* -------------------------------------------------- drawing helpers */
static uint8_t oam_used, seed = 0x5D;
void db_put(uint8_t x, uint8_t y, uint8_t t, uint8_t a) DB_BANKED {
  uint16_t i = ((uint16_t)y << 5) + x;
  MAP[i] = t;
  ATT[i] = a;
  db_row_dirty[y] = 1;
}
static const uint16_t powers[5] = {10000, 1000, 100, 10, 1};
void db_digits(uint8_t x, uint8_t y, uint16_t v, uint8_t width, uint8_t a) DB_BANKED {
  uint8_t i, d;
  uint16_t p;
  for (i = 0; i < 5; i++) {
    p = powers[i];
    d = 0;
    while (v >= p) {
      v -= p;
      d++;
    }
    if (i >= 5 - width)
      db_put(x++, y, 17 + d, a);
  }
}
void db_clear_rows(uint8_t from, uint8_t to) DB_BANKED {
  uint16_t n = (uint16_t)(to - from + 1) << 5;
  memset(MAP + ((uint16_t)from << 5), 0, n);
  memset(ATT + ((uint16_t)from << 5), 0, n);
  for (; from <= to; from++)
    db_row_dirty[from] = 1;
}

void db_pal_apply(void) DB_BANKED {
  uint8_t i;
  for (i = 0; i < 64; i++)
    PAL_LIVE[i] = db_fade >= 8 ? PAL_TARGET[i] : db_scale(PAL_TARGET[i], db_fade);
  db_pal_dirty = 0xFFFF;
}
void db_pal_fill(uint8_t slot, uint16_t c) DB_BANKED {
  uint8_t i, k = slot << 2;
  for (i = 1; i < 4; i++)
    PAL_LIVE[k + i] = c;
  db_pal_dirty |= (uint16_t)1 << slot;
}

/* ------------------------------------------------------------ sprites */
/* 16 x 16 objects are two 8 x 16 hardware sprites. */
void spr16(uint8_t tile, int16_t x, int16_t y, uint8_t a) DB_BANKED {
  OAM_item_t *o;
  if (db_oam > 38 || x < -15 || x > 167 || y < -15 || y > 151)
    return;
  o = &shadow_OAM[db_oam];
  db_oam += 2;
  o->y = (uint8_t)(y + 16);
  o->x = (uint8_t)(x + 8);
  o->tile = (a & S_FLIPX) ? tile + 2 : tile;
  o->prop = a;
  o++;
  o->y = (uint8_t)(y + 16);
  o->x = (uint8_t)(x + 16);
  o->tile = (a & S_FLIPX) ? tile : tile + 2;
  o->prop = a;
}
void spr8(uint8_t tile, int16_t x, int16_t y, uint8_t a) DB_BANKED {
  OAM_item_t *o;
  if (db_oam > 39 || x < -7 || x > 167 || y < -15 || y > 151)
    return;
  o = &shadow_OAM[db_oam++];
  o->y = (uint8_t)(y + 16);
  o->x = (uint8_t)(x + 8);
  o->tile = tile;
  o->prop = a;
}
void db_oam_end(void) DB_BANKED {
  uint8_t i;
  for (i = db_oam; i < oam_used; i++)
    shadow_OAM[i].y = 0;
  oam_used = db_oam;
  db_oam = 0;
}
uint8_t db_rand(void) DB_BANKED {
  seed ^= seed << 3;
  seed ^= seed >> 5;
  seed ^= (uint8_t)db_frame;
  seed ^= seed << 1;
  return seed;
}

/* ------------------------------------------------------------- frames */
void db_wait(void) DB_BANKED {
  uint8_t cur;
  db_oam_end();
  vsync();
  db_frame++;
  tick_voice(&lead, 0);
  tick_voice(&bass, 1);
  tick_voice(&drums, 2);
  tick_sfx();
  cur = joypad();
  db_pressed = cur & ~previous;
  previous = cur;
  db_keys = cur;
#ifdef ARCADE
  if ((cur & (J_START | J_SELECT)) == (J_START | J_SELECT)) {
    db_home = 1;
    return;
  }
#endif
  if ((db_pressed & J_SELECT) && !(cur & J_START)) {
    db_muted ^= 1;
    NR51_REG = db_muted ? 0 : 0xFF;
  }
}
void db_fade_to(uint8_t level) DB_BANKED {
  while (db_fade != level && !db_home) {
    db_fade += db_fade < level ? 1 : -1;
    db_pal_apply();
    db_wait();
  }
}
void db_screen_off(void) DB_BANKED {
  uint8_t i;
  db_fade_to(0);
  db_rmode = 0;
  DISPLAY_OFF;
  for (i = 0; i < 40; i++)
    shadow_OAM[i].y = 0;
  db_scx = db_scy = 0;
  SCX_REG = SCY_REG = 0;
}
void db_screen_on(void) DB_BANKED {
  db_pal_apply();
  db_flush_all();
  SCX_REG = db_scx;
  SCY_REG = db_scy;
  WX_REG = db_wx;
  WY_REG = db_wy;
  DISPLAY_ON;
}

static void sound_init(void) {
  uint8_t i;
  NR52_REG = 0x80;
  NR50_REG = 0x77;
  NR51_REG = db_muted ? 0 : 0xFF;
  NR30_REG = 0;
  /* A rounded square wave: warm enough for bass. */
  for (i = 0; i < 16; i++)
    AUD3WAVE[i] = i < 6 ? 0xFF : i < 8 ? 0xCA : i < 14 ? 0x00 : 0x35;
  NR30_REG = 0x80;
}

#ifdef ARCADE
void dreambase_run(void) BANKED
#else
void main(void)
#endif
{
#ifndef ARCADE
  cpu_fast();
#endif
  DISPLAY_OFF;
  db_home = db_oam = 0;
  db_fade = 0;
  db_won = db_score = 0;
  db_diff = 1;
  db_pal_dirty = 0;
  previous = J_A | J_B | J_START | J_SELECT;
  lead.pos = bass.pos = drums.pos = 0;
  sfx_id = noise_wait = 0;
  db_scx = db_scy = 0;
  db_wx = 7;
  db_wy = 144;
  db_map_hi = 0x98;
  VBK_REG = 0;
  SPRITES_8x16;
  SHOW_BKG;
  SHOW_SPRITES;
  HIDE_WIN;
  sound_init();
  art_common();
  db_raster_on();
  splash_enter();
  for (;;) {
    db_wait();
    if (db_home)
      break;
    switch (db_mode) {
    case M_SPLASH:
      splash_step();
      break;
    case M_TITLE:
      title_step();
      break;
    case M_HELP:
      help_step();
      break;
    case M_PLAY:
      play_step();
      break;
    case M_RESULTS:
      results_step();
      break;
    default:
      credits_step();
    }
  }
  /* Return to the anthology launcher: release the hardware we claimed. */
  db_raster_off();
  db_oam = 0;
  db_oam_end();
  HIDE_WIN;
  NR52_REG = 0;
}
