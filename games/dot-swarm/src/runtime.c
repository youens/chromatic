#include "runtime.h"
#include "assets.h"
#include <string.h>
uint8_t __at(0xD000) tiles[1024];
uint8_t __at(0xD400) colors[1024];
uint8_t keys, pressed, phase = 0, health, stage, won, ended;
uint16_t ticks, score, best = 0, seed = 12345;
static uint8_t previous, muted = 0, half = 0, title_ready = 1;
static uint16_t clock_ticks = 0;
void backdrop(void) {
  memcpy(tiles, scene_map, 1024);
  memcpy(colors, scene_colors, 1024);
}
void screen(uint8_t t, uint8_t p) {
  memset(tiles, t, 1024);
  memset(colors, p, 1024);
}
void label(uint8_t x, uint8_t y, const char *s, uint8_t p) {
  while (*s && x < 20) {
    tile(x++, y, (uint8_t)*s - 31, p);
    ++s;
  }
}
void number(uint8_t x, uint8_t y, uint16_t n, uint8_t w, uint8_t p) {
  while (w) {
    tile(x + --w, y, 17 + n % 10, p);
    n /= 10;
  }
}
void hide_all(void) {
  uint8_t i;
  for (i = 0; i < 40; i++)
    hide_sprite(i);
}
void sprite(uint8_t id, uint8_t t, int16_t x, int16_t y, uint8_t p) {
  if (x < -7 || x > 159 || y < -7 || y > 143) {
    hide_sprite(id);
    return;
  }
  set_sprite_tile(id, t);
  set_sprite_prop(id, p);
  move_sprite(id, x + 8, y + 16);
}
void actor(uint8_t id, uint8_t t, int16_t x, int16_t y, uint8_t p,
           uint8_t flip) {
  uint8_t i;
  for (i = 0; i < 4; i++)
    sprite(id + i, t + (flip ? (i ^ 2) : i), x + (i & 1) * 8, y + (i >> 1) * 8,
           p | (flip ? S_FLIPY : 0));
}
void tone(uint16_t f, uint8_t e) {
  if (muted)
    return;
  NR10_REG = 0;
  NR11_REG = 0x80;
  NR12_REG = e;
  NR13_REG = f;
  NR14_REG = 0x80 | (f >> 8);
}
void noise(uint8_t pitch) {
  if (muted)
    return;
  NR41_REG = 0x10;
  NR42_REG = 0x73;
  NR43_REG = pitch;
  NR44_REG = 0x80;
}
void points(uint16_t n) {
  if (score > 65535u - n)
    score = 65535u;
  else
    score += n;
}
uint16_t random16(void) {
  seed ^= seed << 7;
  seed ^= seed >> 9;
  seed ^= seed << 8;
  return seed;
}
uint16_t distance(int16_t a, int16_t b) { return a > b ? a - b : b - a; }
void hud(const char *s, uint16_t n) {
  uint8_t i;
  label(0, 0, "SCORE", 4);
  number(0, 1, score, 5, 1);
  label(13, 0, s, 4);
  number(15, 1, n, 4, 2);
  for (i = 0; i < 6; i++)
    tile(7 + i, 0, i < health ? HEART : 0, 3);
}
static void flush(void) {
  VBK_REG = 0;
  HDMA1_REG = 0xD0;
  HDMA2_REG = 0;
  HDMA3_REG = 0x18;
  HDMA4_REG = 0;
  HDMA5_REG = 63;
  VBK_REG = 1;
  HDMA1_REG = 0xD4;
  HDMA2_REG = 0;
  HDMA3_REG = 0x18;
  HDMA4_REG = 0;
  HDMA5_REG = 63;
  VBK_REG = 0;
}
static void title(void) {
  uint8_t x, y;
  screen(0, 0);
  hide_all();
  for (y = 0; y < 18; y++)
    for (x = 0; x < 20; x++)
      if ((x * 13 + y * 7) % 29 == 0)
        tile(x, y, STAR, 4);
  for (y = 0; y < 6; y++)
    for (x = 0; x < 20; x++)
      tile(x, y + 3, title_map[y * 20 + x], y < 3 ? 1 : 2);
  for (y = 9; y < 18; y++)
    for (x = 0; x < 20; x++) {
      uint8_t roof = 10 + (x * 7 % 5);
      if (y >= roof) tile(x, y, y == roof ? 75 : 74, 6);
    }
  label(1, 0, "OPENAI", 1);
  label(11, 0, "DEVDAY", 2);
  label(4, 2, "TOKYO NIGHTS", 4);
  label(2, 10, "GLOW. GROW. GO.", 5);
  label(2, 13, "A / START PLAY", 1);
  label(3, 15, "B HOW TO PLAY", 4);
  label(3, 17, "BEST", 4);
  number(9, 17, best, 5, 1);
  actor(0, 0, 72, 88 + (clock_ticks & 16 ? 1 : 0), 1, 0);
}
static void help(void) {
  screen(0, 0);
  hide_all();
  label(1, 1, game_title, 1);
  label(1, 4, game_controls1, 2);
  label(1, 6, game_controls2, 2);
  label(1, 9, game_goal, 4);
  label(1, 12, "START PAUSE", 0);
  label(1, 14, "SELECT SOUND", 0);
  label(2, 17, "A / START PLAY", 1);
}
static void results(void) {
  screen(0, 0);
  hide_all();
  label(2, 2, won ? "MISSION COMPLETE" : "ONE MORE TRY?", 1);
  label(3, 5, game_title, 4);
  label(4, 8, "FINAL SCORE", 2);
  number(6, 10, score, 5, 1);
  label(2, 14, "A / START AGAIN", 2);
  label(3, 16, "B TITLE SCREEN", 4);
}
static void start(void) {
  title_ready = 0;
  ticks = score = won = ended = 0;
  stage = 1;
  health = 3;
  seed = clock_ticks ^ 0xACED;
  if (!seed)
    seed = 1;
  game_reset();
  phase = 2;
  hide_all();
}
void main(void) {
  uint8_t current;
  DISPLAY_OFF;
  cpu_fast();
  set_bkg_data(0, 192, bg_data);
  set_bkg_data(192, TITLE_COUNT, title_data);
  set_sprite_data(0, SPRITE_COUNT, sprite_data);
  set_bkg_palette(0, 8, game_palette);
  set_sprite_palette(0, 8, game_palette);
  SPRITES_8x8;
  SHOW_BKG;
  SHOW_SPRITES;
  HIDE_WIN;
  NR52_REG = 0x80;
  NR50_REG = 0x66;
  NR51_REG = 0xFF;
  title();
  flush();
  DISPLAY_ON;
  for (;;) {
    vsync();
    flush();
    clock_ticks++;
    half ^= 1;
    if (half)
      continue;
    current = joypad();
    pressed = current & ~previous;
    keys = current;
    previous = current;
    if (pressed & J_SELECT) {
      muted = !muted;
      NR51_REG = (muted || phase == 3) ? 0 : 0xFF;
    }
    if (phase == 0) {
      if (!title_ready) {
        title();
        title_ready = 1;
      }
      if (pressed & (J_A | J_START))
        start();
      else if (pressed & J_B) {
        title_ready = 0;
        phase = 1;
        help();
      }
    } else if (phase == 1) {
      if (pressed & (J_A | J_START))
        start();
      else if (pressed & J_B)
        phase = 0;
    } else if (phase == 2) {
      if (pressed & J_START) {
        phase = 3;
        NR51_REG = 0;
        memset(tiles + 544, 0, 20);
        label(1, 17, "PAUSED: START GO", 1);
      } else {
        ticks++;
        game_update();
        hide_all();
        memset(tiles + 544, 0, 20);
        game_draw();
        if (ended) {
          if (score > best)
            best = score;
          phase = 4;
          results();
          tone(won ? 1900 : 1450, 0xA5);
        }
      }
    } else if (phase == 3) {
      if (pressed & J_START) {
        phase = 2;
        NR51_REG = muted ? 0 : 0xFF;
      }
    } else if (phase == 4) {
      if (pressed & (J_A | J_START))
        start();
      else if (pressed & J_B)
        phase = 0;
    }
  }
}
