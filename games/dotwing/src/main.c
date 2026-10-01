/* Dotwing: a native, offline Game Boy Color arcade shooter. */
#ifndef ARCADE
#pragma bank 1
#endif
#include "dotwing.h"
#include "art_ids.h"

static uint8_t previous;
static uint16_t credited;
static const uint16_t melody[32] = {
  1751,1821,1860,0,1891,1860,1821,0,1751,1821,1860,1891,1929,0,1891,0,
  1721,1798,1849,0,1882,1849,1798,0,1751,1821,1860,1891,1860,1821,1751,0
};

static void credit_tokens(void) {
  uint16_t n = dw_run_tokens - credited;
  if (dw_profile.tokens > 65535u - n) dw_profile.tokens = 65535u;
  else dw_profile.tokens += n;
  credited = dw_run_tokens;
  if (dw_score > dw_best) dw_best = dw_score;
  dw_save();
}

void dw_finish(uint8_t victory) BANKED {
  dw_victory = victory;
  credit_tokens();
  dw_state = DW_RESULTS;
  dw_screen();
  dw_tone(victory ? 1950 : 1540, 0xA5);
}

void dw_start(void) BANKED {
  uint8_t i, next = dw_state == DW_SHOP;
  if (next) {
    ++dw_sector;
    if (dw_hp < 3u + dw_profile.shield) ++dw_hp;
  } else {
    dw_sector = 1;
    dw_score = dw_kills = dw_run_tokens = credited = 0;
    dw_hp = 3u + dw_profile.shield;
    dw_energy = 60;
    dw_power = dw_combo = dw_victory = 0;
    dw_seed = dw_clock ^ 0xD071u;
    if (!dw_seed) dw_seed = 1;
  }
  dw_frame = dw_stage_frame = 0;
  dw_boss_active = dw_boss_hp = dw_boss_max = dw_burst = 0;
  dw_invincible = 120;
  dw_px = 72; dw_py = 94;
  dw_focus = 0;
  dw_menu_row = 0;
  for (i = 0; i < DW_ENEMIES; ++i) dw_enemies[i].active = 0;
  for (i = 0; i < DW_SHOTS; ++i) dw_shots[i].active = 0;
  for (i = 0; i < DW_BULLETS; ++i) dw_bullets[i].active = 0;
  for (i = 0; i < DW_DROPS; ++i) dw_drops[i].active = 0;
  dw_state = DW_TAKEOFF;
  dw_dirty = 1;
  dw_clear(0);
  dw_palettes(dw_sector);
  dw_dot_art();
  dw_sky(dw_sector);
  dw_hud();
  dw_draw();
  SHOW_WIN;
  DISPLAY_ON;
  dw_tone(1821, 0x82);
}

static void music_tick(void) {
  uint16_t f;
  uint8_t step;
  if (dw_muted || dw_state == DW_PAUSE || (dw_clock % 12u)) return;
  step = (dw_clock / 12u) & 31u;
  f = melody[(step + (dw_sector > 2 ? 8 : 0)) & 31u];
  NR21_REG = 0x80;
  NR22_REG = f ? 0x43 : 0;
  NR23_REG = (uint8_t)f;
  NR24_REG = 0x80u | (uint8_t)(f >> 8);
  if (!(step & 3u)) {
    NR41_REG = 0x10; NR42_REG = 0x21;
    NR43_REG = 0x10; NR44_REG = 0x80;
  }
}

/* Called after the last boss hit, outside the update module's bank. */
static void sector_complete(void) {
  dw_points(500u + (uint16_t)dw_sector * 100u);
  if (dw_profile.cleared < dw_sector) dw_profile.cleared = dw_sector;
  credit_tokens();
  dw_noise(0x35);
  if (dw_sector == 4) dw_finish(1);
  else {
    dw_state = DW_SHOP;
    dw_menu_row = 0;
    dw_screen();
  }
}

void dotwing_run(void) BANKED {
  uint8_t current;
  DISPLAY_OFF;
  previous = 0;
  dw_clock = dw_frame = 0;
  dw_muted = dw_keys = dw_pressed = 0;
  dw_sector = 1;
  dw_state = DW_TITLE;
  dw_load();
  /* Signed BG tiles at 0x9000 leave OBJ tiles at 0x8000 intact. */
  LCDC_REG = 0x47;
  dw_art_load();
  dw_palettes(1);
  dw_dot_art();
  NR52_REG = 0x80; NR50_REG = 0x66; NR51_REG = 0xFF;
  dw_screen();
  for (;;) {
    vsync();
    current = joypad();
#ifdef ARCADE
    if ((current & (J_START | J_SELECT)) == (J_START | J_SELECT)) {
      if (dw_state == DW_FLIGHT || dw_state == DW_PAUSE || dw_state == DW_TAKEOFF)
        credit_tokens();
      DISPLAY_OFF;
      dw_hide();
      HIDE_WIN;
      SCX_REG = SCY_REG = 0;
      NR51_REG = 0;
      return;
    }
#endif
    dw_keys = current;
    dw_pressed = current & ~previous;
    previous = current;
    ++dw_clock;
    if (dw_pressed & J_SELECT) {
      dw_muted ^= 1u;
      NR51_REG = dw_muted || dw_state == DW_PAUSE ? 0 : 0xFF;
    }
    if (dw_state == DW_FLIGHT) {
      if (dw_pressed & J_START) {
        dw_state = DW_PAUSE;
        dw_screen();
        NR51_REG = 0;
      } else {
        ++dw_frame;
        dw_update();
        if (dw_state == DW_FLIGHT && dw_boss_active == 2) sector_complete();
        if (dw_state == DW_FLIGHT) {
          dw_draw();
          if (dw_dirty || !(dw_frame & 7u)) dw_hud();
        }
      }
    } else if (dw_state == DW_TAKEOFF) {
      ++dw_stage_frame;
      dw_draw();
      if (dw_stage_frame < 48) {
        dw_actor(36, DW_SPR_PILOT, 72, 46 + dw_stage_frame, 0);
      }
      if (dw_stage_frame >= 72) {
        dw_stage_frame = 0;
        dw_state = DW_FLIGHT;
        dw_dirty = 1;
      }
    } else {
      dw_menu_update();
      if (dw_state != DW_FLIGHT && dw_state != DW_TAKEOFF) dw_menu_animate();
      if (dw_state == DW_FLIGHT) NR51_REG = dw_muted ? 0 : 0xFF;
    }
    music_tick();
  }
}

#ifndef ARCADE
void main(void) {
  cpu_fast();
  dotwing_run();
}
#endif
