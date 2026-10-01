/* Dotwing: a native, offline Game Boy Color arcade shooter.
   Entry point, frame loop, token banking and the sound driver. */
#ifndef ARCADE
#pragma bank 1
#endif
#include "dotwing.h"

BANKREF(dw_sound)
#include "music_data.h"

/* ------------------------------------------------------------- sound */
typedef struct {
  const uint8_t *start, *pos;
  const uint8_t *rep[2];
  uint8_t rep_n[2], depth;
  uint8_t wait, len, gate, ins, wave;
} Voice;
static Voice voices[4];
volatile uint8_t dw_sfx_req, dw_song_req;
static volatile uint8_t sfx4_req;
static const uint8_t *sfx_pos[2];
static uint8_t sfx_wait[2], sfx_prio[2];

static void silence(uint8_t ch) {
  if (ch == 0) NR12_REG = 0;
  else if (ch == 1) NR22_REG = 0;
  else if (ch == 2) NR32_REG = 0;
  else NR42_REG = 0;
}

static uint16_t freq(uint8_t note) {
  if (note < 36) note = 36;
  else if (note > 107) note = 107;
  return note_freq[note - 36];
}

static void load_wave(uint8_t w) {
  uint8_t i;
  const uint8_t *src = waves[w];
  NR30_REG = 0;
  for (i = 0; i < 16; ++i) ((volatile uint8_t *)0xFF30)[i] = src[i];
  NR30_REG = 0x80;
}

static void play(uint8_t ch, Voice *v, uint8_t note) {
  const uint8_t *in = instruments + ((uint16_t)v->ins << 2);
  uint16_t f;
  v->gate = in[2];
  if (ch == 3) {
    if (sfx_pos[1]) return;
    NR41_REG = 0;
    NR42_REG = drum_kit[note << 1];
    NR43_REG = drum_kit[(note << 1) + 1];
    NR44_REG = 0x80;
    return;
  }
  if (ch == 2) {
    if (v->wave != (in[0] & 0x7F)) {
      v->wave = in[0] & 0x7F;
      load_wave(v->wave);
    }
    f = freq(note + 12u);
    NR30_REG = 0x80;
    NR32_REG = in[3];
    NR33_REG = (uint8_t)f;
    NR34_REG = 0x80 | (uint8_t)(f >> 8);
    return;
  }
  f = freq(note);
  if (ch == 0) {
    if (sfx_pos[0]) return;
    NR10_REG = 0;
    NR11_REG = in[0] << 6;
    NR12_REG = in[1];
    NR13_REG = (uint8_t)f;
    NR14_REG = 0x80 | (uint8_t)(f >> 8);
  } else {
    NR21_REG = in[0] << 6;
    NR22_REG = in[1];
    NR23_REG = (uint8_t)f;
    NR24_REG = 0x80 | (uint8_t)(f >> 8);
  }
}

static void voice_tick(uint8_t ch, Voice *v) {
  uint8_t b;
  if (!v->pos) return;
  if (v->gate && !--v->gate && !(ch == 0 && sfx_pos[0])) silence(ch);
  if (v->wait && --v->wait) return;
  for (;;) {
    b = *v->pos++;
    if (b < 0x80) {
      if (b) play(ch, v, b);
      else if (!(ch == 0 && sfx_pos[0]) && ch != 3) silence(ch);
      v->wait = v->len;
      return;
    }
    if (b < 0xC0) {
      v->len = (b & 0x3F) + 1;
      continue;
    }
    switch (b) {
      case 0xC0:
        v->wait = v->len;
        return;
      case 0xC1:
        v->ins = *v->pos++;
        break;
      case 0xC3:
        v->pos = v->start + (v->pos[0] | ((uint16_t)v->pos[1] << 8));
        break;
      case 0xC6:
        if (v->depth < 2) {
          v->rep_n[v->depth] = *v->pos++;
          v->rep[v->depth++] = v->pos;
        } else ++v->pos;
        break;
      case 0xC7:
        if (v->depth) {
          if (--v->rep_n[v->depth - 1]) v->pos = v->rep[v->depth - 1];
          else --v->depth;
        }
        break;
      default:
        v->pos = 0;
        if (!(ch == 0 && sfx_pos[0])) silence(ch);
        return;
    }
  }
}

static void start_song(uint8_t id) {
  uint8_t ch;
  Voice *v = voices;
  for (ch = 0; ch < 4; ++ch, ++v) {
    v->start = v->pos = id ? songs[id][ch] : 0;
    v->wait = 1;
    v->len = 12;
    v->depth = v->gate = v->ins = 0;
    v->wave = 0xFF;
    if (!(ch == 0 && sfx_pos[0]) && !(ch == 3 && sfx_pos[1])) silence(ch);
  }
}

static void sfx_tick(uint8_t k) {
  const uint8_t *p = sfx_pos[k];
  if (!p) return;
  if (sfx_wait[k] && --sfx_wait[k]) return;
  if (!p[0]) {
    sfx_pos[k] = 0;
    if (k) NR42_REG = 0;
    else NR12_REG = 0;
    return;
  }
  if (!k) {
    NR10_REG = p[1];
    NR11_REG = p[2];
    NR12_REG = p[3];
    NR13_REG = p[4];
    NR14_REG = p[5];
  } else {
    NR41_REG = 0;
    NR42_REG = p[2];
    NR43_REG = p[3];
    NR44_REG = 0x80;
  }
  sfx_wait[k] = p[0];
  sfx_pos[k] = p + 6;
}

static void sfx_start(uint8_t k, uint8_t id) {
  uint8_t prio = sfx_meta[(id << 1) + 1];
  if (sfx_pos[k] && prio < sfx_prio[k]) return;
  sfx_pos[k] = sfx_data[id];
  sfx_wait[k] = 0;
  sfx_prio[k] = prio;
}

/* Runs from the VBlank interrupt with this bank mapped by dw_vbl(). */
void dw_sound_tick(void) {
  uint8_t ch;
  if (dw_song_req) {
    start_song(dw_song_req & 0x7F);
    dw_song_req = 0;
  }
  if (dw_sfx_req) {
    sfx_start(0, dw_sfx_req);
    dw_sfx_req = 0;
  }
  if (sfx4_req) {
    sfx_start(1, sfx4_req);
    sfx4_req = 0;
  }
  if (sfx_pos[0]) sfx_tick(0);
  if (sfx_pos[1]) sfx_tick(1);
  /* Most frames only count down: handle that without a call. */
  for (ch = 0; ch < 4; ++ch) {
    Voice *v = &voices[ch];
    if (!v->pos) continue;
    if (v->wait > 1 && v->gate != 1) {
      --v->wait;
      if (v->gate) --v->gate;
      continue;
    }
    voice_tick(ch, v);
  }
}

void dw_sfx(uint8_t id) BANKED {
  uint8_t prio = sfx_meta[(id << 1) + 1];
  if (sfx_meta[id << 1] == 4) {
    if (!sfx4_req || prio >= sfx_meta[(sfx4_req << 1) + 1]) sfx4_req = id;
  } else if (!dw_sfx_req || prio >= sfx_meta[(dw_sfx_req << 1) + 1]) {
    dw_sfx_req = id;
  }
}

void dw_song(uint8_t id) BANKED {
  dw_song_req = id | 0x80;
}

/* -------------------------------------------------------- the frame loop */
void dw_vbl(void);
static uint16_t credited;

static void credit_tokens(void) {
  uint16_t n = dw_run_tokens - credited;
  if (dw_profile.tokens > 65535u - n) dw_profile.tokens = 65535u;
  else dw_profile.tokens += n;
  credited = dw_run_tokens;
  if (dw_score > dw_best) {
    dw_best = dw_score;
    dw_new_best = 1;
  }
  dw_save();
}

/* Called when a sortie begins (credited resets) or a sector is banked. */
void dw_bank(uint8_t fresh) BANKED {
  if (fresh) credited = 0;
  else credit_tokens();
}

void dw_finish(uint8_t victory) BANKED {
  dw_victory = victory;
  credit_tokens();
  dw_song(victory ? SONG_VICTORY : SONG_OVER);
  dw_fade_to(0);
  dw_state = DW_RESULTS;
  dw_screen();
}

void dotwing_run(void) BANKED {
  uint8_t current, previous = 0xFF, lcdc;
  lcdc = LCDC_REG;
  DISPLAY_OFF;
  dw_clock = dw_frame = 0;
  dw_muted = dw_keys = dw_pressed = 0;
  dw_sector = 1;
  dw_fade = 0;
  dw_flash_mask = dw_shake = dw_scx = dw_scy = 0;
  dw_new_best = 0;
  dw_load();
  /* Signed BG tiles at 0x9000 leave OBJ tiles at 0x8000 intact. 8x16
     objects, window map at 0x9C00, BG map at 0x9800. */
  LCDC_REG = 0x47;
  NR52_REG = 0x80;
  NR50_REG = 0x77;
  NR51_REG = 0xFF;
  dw_sfx_req = sfx4_req = 0;
  sfx_pos[0] = sfx_pos[1] = 0;
  start_song(0);
  CRITICAL {
    add_VBL(dw_vbl);
  }
  dw_load_common();
  dw_load_menu_sprites();
  dw_pal_menu((const uint8_t *)&dw_profile);
  dw_pal_now();
  dw_oam_begin();
  dw_oam_flip();
  dw_state = DW_TITLE;
  dw_song(SONG_TITLE);
  dw_screen();
  for (;;) {
    dw_oam_begin();
    current = joypad();
#ifdef ARCADE
    if ((current & (J_START | J_SELECT)) == (J_START | J_SELECT)) {
      if (dw_state == DW_FLIGHT || dw_state == DW_PAUSE || dw_state == DW_TAKEOFF)
        credit_tokens();
      dw_song(SONG_NONE);
      dw_fade_to(0);
      CRITICAL {
        remove_VBL(dw_vbl);
      }
      NR51_REG = 0;
      NR52_REG = 0;
      DISPLAY_OFF;
      _shadow_OAM_base = 0xC0;
      for (current = 0; current < 40; ++current) shadow_OAM[current].y = 0;
      VBK_REG = 0;
      HIDE_WIN;
      SCX_REG = SCY_REG = 0;
      LCDC_REG = lcdc & 0x7F;
      return;
    }
#endif
    dw_keys = current;
    dw_pressed = current & ~previous;
    previous = current;
    ++dw_clock;
    if (dw_pressed & J_SELECT) {
      dw_muted ^= 1u;
      NR51_REG = dw_muted ? 0 : 0xFF;
    }
    if (dw_state == DW_FLIGHT || dw_state == DW_TAKEOFF || dw_state == DW_PAUSE) dw_update();
    else dw_menu_update();
    dw_sync();
  }
}

#ifndef ARCADE
void main(void) {
  cpu_fast();
  dotwing_run();
}
#endif
