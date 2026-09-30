#include "runtime.h"
const char game_title[] = "MOONTHREAD", game_tagline[] = "GRAVITY IS A THREAD",
           game_controls1[] = "LEFT RIGHT DRIFT",
           game_controls2[] = "A FLIP  B BRAKE",
           game_goal[] = "3 STARS THEN EXIT";
#define C RGB
const uint16_t game_palette[32] = {
    C(3, 3, 9),    C(8, 7, 17),   C(15, 14, 22), C(29, 29, 30), C(3, 3, 9),
    C(10, 10, 18), C(31, 20, 7),  C(30, 30, 28), C(3, 3, 9),    C(9, 10, 19),
    C(20, 18, 31), C(29, 25, 31), C(3, 3, 9),    C(19, 5, 14),  C(28, 11, 21),
    C(31, 21, 26), C(3, 3, 9),    C(10, 9, 20),  C(16, 14, 26), C(25, 22, 30),
    C(3, 3, 9),    C(13, 11, 23), C(19, 17, 28), C(27, 25, 30), C(3, 3, 9),
    C(4, 16, 16),  C(10, 25, 23), C(23, 31, 29), C(3, 3, 9),    C(18, 11, 5),
    C(29, 20, 8),  C(31, 29, 19)};
int16_t px, py, vx, vy;
int8_t gravity;
uint8_t stars, invincible, flip_flash, dirty;
const uint8_t starx[3] = {35, 79, 123};
uint8_t star_y(uint8_t i) { return ((i + stage) & 1) ? 42 : 102; }
void spawn(void) {
  dirty = 1;
  px = 12 * 16;
  py = 102 * 16;
  vx = vy = 0;
  gravity = 1;
  invincible = 90;
}
void game_reset(void) {
  health = 6;
  stars = 0;
  spawn();
  flip_flash = 0;
}
void game_update(void) {
  uint8_t i;
  int16_t hx, hy;
  int16_t oldy = py;
  if (invincible)
    invincible--;
  if (flip_flash)
    flip_flash--;
  if (pressed & J_A) {
    gravity = -gravity;
    vy = gravity * 12;
    flip_flash = 8;
    tone(gravity < 0 ? 1900 : 1650, 0x83);
  }
  if (keys & J_LEFT)
    vx -= 5;
  else if (keys & J_RIGHT)
    vx += 5;
  else
    vx = vx * 3 / 4;
  if (keys & J_B)
    vx = vx / 2;
  if (vx > 34)
    vx = 34;
  if (vx < -34)
    vx = -34;
  px += vx;
  if (px < 4 * 16)
    px = 4 * 16;
  if (px > 140 * 16)
    px = 140 * 16;
  vy += gravity * 4;
  if (vy > 42)
    vy = 42;
  if (vy < -42)
    vy = -42;
  py += vy;
  if (py < 32 * 16) {
    py = 32 * 16;
    vy = 0;
  }
  if (py > 112 * 16) {
    py = 112 * 16;
    vy = 0;
  }
  /* Two suspended ledges block vertical travel, with open routes at their ends.
   */
  for (i = 0; i < 2; i++) {
    hx = 40 + i * 56;
    hy = 72 + (stage & 1 ? 0 : 8);
    if ((px >> 4) + 13 > hx && (px >> 4) + 3 < hx + 24) {
      if (vy > 0 && oldy + 16 * 16 <= hy * 16 && py + 16 * 16 > hy * 16) {
        py = (hy - 16) * 16;
        vy = 0;
      } else if (vy < 0 && oldy >= ((hy + 8) * 16) && py < (hy + 8) * 16) {
        py = (hy + 8) * 16;
        vy = 0;
      }
    }
  }
  for (i = 0; i < 3; i++)
    if (!(stars & (1 << i)) && distance((px >> 4) + 8, starx[i]) < 13 &&
        distance((py >> 4) + 8, star_y(i)) < 14) {
      stars |= 1 << i;
      points(100);
      tone(1750 + i * 65, 0x93);
    }
  hx = 20 + (ticks * (1 + stage / 3) % 120);
  hy = (stage & 1) ? 95 : 51;
  if (!invincible && distance((px >> 4) + 8, hx) < 11 &&
      distance((py >> 4) + 8, hy) < 12) {
    health--;
    noise(0x64);
    spawn();
  }
  if (stars == 7 && px > 134 * 16 && py < 57 * 16) {
    points(500 + (3600 - ticks) / 30);
    stage++;
    tone(1970, 0xA4);
    if (stage > 6) {
      ended = won = 1;
    } else {
      stars = 0;
      spawn();
    }
  }
  if (!health || ticks >= 3600)
    ended = 1;
}
void game_draw(void) {
  uint8_t x, i;
  int16_t hx;
  if (dirty) {
    backdrop();
    for (x = 0; x < 20; x++) {
      tile(x, 3, WALL, 5);
      tile(x, 16, WALL, 5);
    }
    for (i = 0; i < 2; i++)
      for (x = 0; x < 3; x++)
        tile(5 + i * 7 + x, 9 + (stage & 1 ? 0 : 1), LINE, 2);
    dirty = 0;
  }
  for (i = 0; i < 3; i++)
    if (!(stars & (1 << i)))
      sprite(4 + i, 8, starx[i] - 4, star_y(i) - 4, 7);
  hx = 20 + (ticks * (1 + stage / 3) % 120);
  actor(8, 4, hx - 8, (stage & 1) ? 87 : 43, 3, 0);
  tile(18, 5, GATE, stars == 7 ? 6 : 4);
  tile(18, 6, GATE, stars == 7 ? 6 : 4);
  if (!invincible || (ticks & 2))
    actor(0, 0, px >> 4, py >> 4, 1, gravity < 0);
  if (flip_flash) {
    sprite(16, 8, (px >> 4) - 5, (py >> 4) + 6, 6);
    sprite(17, 8, (px >> 4) + 17, (py >> 4) + 6, 6);
  }
  hud("ROOM", stage);
  label(0, 17, gravity < 0 ? "GRAVITY UP" : "GRAVITY DOWN", 2);
  number(13, 17, 120 - ticks / 30, 3, 4);
  number(17, 17, (stars & 1) + ((stars >> 1) & 1) + ((stars >> 2) & 1), 1, 7);
  label(18, 17, "/3", 4);
}
