#include "runtime.h"
const char game_title[] = "ORBIT CHOIR", game_tagline[] = "MAKE THE STARS SING",
           game_controls1[] = "LEFT RIGHT ORBIT",
           game_controls2[] = "A STRIKE  B NOVA",
           game_goal[] = "HIT NOTES ON RING";
#define C RGB
const uint16_t game_palette[32] = {
    C(2, 3, 9),    C(7, 9, 18),   C(11, 16, 24), C(27, 29, 31), C(2, 3, 9),
    C(7, 12, 22),  C(18, 24, 31), C(30, 29, 26), C(2, 3, 9),    C(4, 14, 18),
    C(11, 26, 25), C(23, 31, 29), C(2, 3, 9),    C(17, 7, 17),  C(27, 14, 23),
    C(31, 23, 18), C(2, 3, 9),    C(8, 8, 18),   C(14, 14, 26), C(25, 23, 31),
    C(2, 3, 9),    C(5, 9, 20),   C(8, 15, 27),  C(18, 26, 31), C(2, 3, 9),
    C(16, 10, 8),  C(29, 20, 13), C(31, 29, 22), C(2, 3, 9),    C(11, 11, 20),
    C(19, 22, 29), C(30, 31, 31)};
const int8_t sx[8] = {0, 7, 10, 7, 0, -7, -10, -7},
             sy[8] = {-10, -7, 0, 7, 10, 7, 0, -7};
const uint16_t notes[8] = {1546, 1601, 1650, 1694, 1733, 1767, 1798, 1825};
uint8_t dirty;
const uint8_t ringx[8] = {80, 112, 126, 112, 80, 48, 34, 48},
              ringy[8] = {34, 48, 80, 112, 126, 112, 80, 48};
const uint8_t diagonal[66] = {
    0,  0,  1,  2,  2,  3,  4,  4,  5,  6,  7,  7,  8,  9,  9,  10, 11,
    11, 12, 13, 14, 14, 15, 16, 16, 17, 18, 18, 19, 20, 21, 21, 22, 23,
    23, 24, 25, 25, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 35,
    35, 36, 37, 37, 38, 39, 39, 40, 41, 42, 42, 43, 44, 44, 45};
uint8_t lane, combo, charge, flash, move_wait, hit_wait, shield;
uint8_t nlane[6], radius[6], active[6];
uint16_t hits;
void game_reset(void) {
  uint8_t i;
  dirty = 1;
  lane = combo = charge = flash = move_wait = hit_wait = shield = 0;
  hits = 0;
  health = 6;
  for (i = 0; i < 6; i++)
    active[i] = 0;
}
void game_update(void) {
  uint8_t i, found = 0;
  if (move_wait)
    move_wait--;
  if (hit_wait)
    hit_wait--;
  if (shield)
    shield--;
  if (flash)
    flash--;
  if (!move_wait) {
    if (keys & J_LEFT) {
      lane = (lane + 7) & 7;
      move_wait = 3;
    } else if (keys & J_RIGHT) {
      lane = (lane + 1) & 7;
      move_wait = 3;
    }
  }
  if ((pressed & J_A) && !hit_wait) {
    hit_wait = 4;
    flash = 4;
    for (i = 0; i < 6; i++)
      if (active[i] && nlane[i] == lane && radius[i] >= 39 && radius[i] <= 53) {
        active[i] = 0;
        hits++;
        if (combo < 64)
          combo++;
        if (charge < 8)
          charge++;
        points((distance(radius[i], 46) < 4 ? 100 : 60) * (1 + combo / 8));
        tone(notes[lane], 0xA4);
        found = 1;
      }
    if (!found) {
      combo = 0;
      noise(0x22);
    }
  }
  if ((pressed & J_B) && charge == 8) {
    charge = 0;
    flash = 15;
    shield = 18;
    for (i = 0; i < 6; i++)
      if (active[i]) {
        active[i] = 0;
        points(80);
      }
    if (health < 6)
      health++;
    tone(1980, 0xB5);
  }
  if (ticks % ((ticks > 1200) ? 16 : ((ticks > 600) ? 20 : 24)) == 0) {
    for (i = 0; i < 6; i++)
      if (!active[i]) {
        active[i] = 1;
        radius[i] = 65;
        nlane[i] = random16() & 7;
        break;
      }
  }
  if (!(ticks & 1))
    for (i = 0; i < 6; i++)
      if (active[i]) {
        radius[i]--;
        if (radius[i] < 34) {
          active[i] = 0;
          combo = 0;
          if (!shield) {
            health--;
            shield = 5;
            noise(0x54);
          }
        }
      }
  if (!health)
    ended = 1;
  if (ticks >= 1800) {
    won = ended = 1;
    points(hits * 10);
  }
}
void game_draw(void) {
  uint8_t i, r, n;
  int16_t px, py;
  if (dirty) {
    backdrop();
    dirty = 0;
  }
  for (i = 0; i < 8; i++) {
    px = ringx[i];
    py = ringy[i];
    sprite(12 + i, 12, px - 4, py - 4, i == lane ? 6 : 4);
  }
  for (i = 0; i < 6; i++)
    if (active[i]) {
      n = nlane[i];
      r = (n & 1) ? diagonal[radius[i]] : radius[i];
      px = 80 + (sx[n] > 0 ? r : sx[n] < 0 ? -(int16_t)r : 0);
      py = 80 + (sy[n] > 0 ? r : sy[n] < 0 ? -(int16_t)r : 0);
      sprite(4 + i, 13, px - 4, py - 4,
             radius[i] >= 39 && radius[i] <= 53 ? 2 : 3);
    }
  px = ringx[lane];
  py = ringy[lane];
  actor(0, 0, px - 8, py - 8, flash ? 2 : 1, lane < 3 || lane > 5);
  hud("TIME", 60 - ticks / 30);
  label(0, 17, "COMBO", 4);
  number(5, 17, combo, 2, 2);
  label(10, 17, charge == 8 ? "B NOVA!" : "NOVA", charge == 8 ? 6 : 4);
  if (charge < 8)
    number(16, 17, charge, 1, 6);
  if (flash > 5)
    set_bkg_palette_entry(5, 2, C(20, 27, 31));
  else
    set_bkg_palette_entry(5, 2, C(8, 15, 27));
}
