#include "runtime.h"
#include <string.h>
const char game_title[] = "ECHO VAULT", game_tagline[] = "LIGHT MAKES A SOUND",
           game_controls1[] = "D PAD MOVE QUIETLY",
           game_controls2[] = "A SONAR  B DECOY",
           game_goal[] = "3 KEYS THEN EXIT";
#define C RGB
const uint16_t game_palette[32] = {
    C(1, 4, 6),    C(3, 7, 9),    C(7, 12, 14),  C(23, 28, 27), C(1, 4, 6),
    C(4, 11, 12),  C(6, 22, 19),  C(20, 31, 27), C(1, 4, 6),    C(3, 13, 13),
    C(7, 24, 20),  C(24, 31, 24), C(1, 4, 6),    C(16, 5, 6),   C(28, 9, 9),
    C(31, 20, 12), C(1, 4, 6),    C(3, 6, 8),    C(6, 10, 12),  C(10, 16, 16),
    C(1, 4, 6),    C(8, 13, 15),  C(14, 22, 21), C(23, 28, 26), C(1, 4, 6),
    C(12, 9, 3),   C(26, 20, 8),  C(31, 29, 15), C(1, 4, 6),    C(4, 10, 10),
    C(10, 19, 17), C(22, 29, 23)};
uint8_t maze[247], seen[247], stack[247], distmap[247];
uint8_t px, py, gx[2], gy[2], collected, pulse, alert, decoy, dx, dy, energy,
    move_wait, shield;
uint16_t turns;
uint8_t dirty;
const uint8_t kx[3] = {7, 15, 3}, ky[3] = {3, 7, 11};
static uint8_t pos(uint8_t x, uint8_t y) { return y * 19 + x; }
static uint8_t open(int8_t x, int8_t y) {
  return x > 0 && x < 18 && y > 0 && y < 12 && !maze[pos(x, y)];
}
static void chamber(void) {
  uint8_t top = 0, x, y, d, j, n, choices[4], nx, ny;
  int8_t xx, yy;
  const int8_t vx[4] = {0, 1, 0, -1}, vy[4] = {-1, 0, 1, 0};
  memset(maze, 1, sizeof(maze));
  memset(seen, 0, sizeof(seen));
  stack[0] = 20;
  maze[20] = 0;
  while (1) {
    n = stack[top];
    x = n % 19;
    y = n / 19;
    j = 0;
    for (d = 0; d < 4; d++) {
      xx = x + vx[d] * 2;
      yy = y + vy[d] * 2;
      if (xx > 0 && xx < 18 && yy > 0 && yy < 12 && maze[pos(xx, yy)])
        choices[j++] = d;
    }
    if (!j) {
      if (!top)
        break;
      top--;
      continue;
    }
    d = choices[random16() % j];
    nx = x + vx[d] * 2;
    ny = y + vy[d] * 2;
    maze[pos(x + vx[d], y + vy[d])] = 0;
    maze[pos(nx, ny)] = 0;
    stack[++top] = pos(nx, ny);
  }
  for (j = 0; j < 12; j++) {
    x = 2 + random16() % 15;
    y = 2 + random16() % 9;
    if (open(x - 1, y) && open(x + 1, y))
      maze[pos(x, y)] = 0;
    if (open(x, y - 1) && open(x, y + 1))
      maze[pos(x, y)] = 0;
  }
  dirty = 1;
  px = py = 1;
  gx[0] = 17;
  gy[0] = 9;
  gx[1] = 9;
  gy[1] = 11;
  collected = alert = decoy = 0;
  pulse = 5;
  energy = 4;
  shield = 3;
}
void game_reset(void) {
  health = 6;
  turns = 0;
  move_wait = 0;
  chamber();
}
static void guards(void) {
  uint8_t head = 0, tail = 1, i, j, n, x, y, best, next;
  int8_t xx, yy;
  const int8_t vx[4] = {0, 1, 0, -1}, vy[4] = {-1, 0, 1, 0};
  if (alert || decoy) {
    memset(distmap, 255, sizeof(distmap));
    n = pos(decoy ? dx : px, decoy ? dy : py);
    stack[0] = n;
    distmap[n] = 0;
    while (head < tail) {
      n = stack[head++];
      x = n % 19;
      y = n / 19;
      for (j = 0; j < 4; j++) {
        xx = x + vx[j];
        yy = y + vy[j];
        if (open(xx, yy) && distmap[pos(xx, yy)] == 255) {
          next = pos(xx, yy);
          distmap[next] = distmap[n] + 1;
          stack[tail++] = next;
        }
      }
    }
  }
  for (i = 0; i < 2; i++) {
    n = pos(gx[i], gy[i]);
    best = 255;
    next = n;
    for (j = 0; j < 4; j++) {
      xx = gx[i] + vx[j];
      yy = gy[i] + vy[j];
      if (!open(xx, yy))
        continue;
      x = pos(xx, yy);
      y = (alert || decoy) ? distmap[x] : (random16() & 63);
      if (y < best) {
        best = y;
        next = x;
      }
    }
    gx[i] = next % 19;
    gy[i] = next / 19;
  }
}
void game_update(void) {
  int8_t nx = px, ny = py;
  uint8_t i, action = 0, x, y;
  if (move_wait)
    move_wait--;
  if (pressed & J_A) {
    if (energy) {
      energy--;
      pulse = 7;
      alert = 6;
      action = 1;
      tone(1850, 0x84);
    }
  } else if (pressed & J_B) {
    if (energy) {
      energy--;
      decoy = 5;
      dx = px;
      dy = py;
      action = 1;
      tone(1560, 0x83);
    }
  } else if (!move_wait && (keys & (J_LEFT | J_RIGHT | J_UP | J_DOWN))) {
    if (keys & J_LEFT)
      nx--;
    else if (keys & J_RIGHT)
      nx++;
    else if (keys & J_UP)
      ny--;
    else
      ny++;
    if (open(nx, ny)) {
      px = nx;
      py = ny;
      action = 1;
    }
    move_wait = 5;
  }
  if (action) {
    dirty = 1;
    turns++;
    if (pulse)
      pulse--;
    if (alert)
      alert--;
    if (decoy)
      decoy--;
    if (shield)
      shield--;
    if (turns % 6 == 0 && energy < 4)
      energy++;
    for (i = 0; i < 3; i++)
      if (!(collected & (1 << i)) && px == kx[i] && py == ky[i]) {
        collected |= 1 << i;
        points(200);
        energy = 4;
        tone(1950, 0x94);
      } /* Quiet movement lets guards move at half the player's rate. */
    if (alert || decoy || !(turns & 1))
      guards();
    for (i = 0; i < 2; i++)
      if (!shield && px == gx[i] && py == gy[i]) {
        health--;
        shield = 10;
        px = py = 1;
        alert = 0;
        noise(0x64);
      }
    if (px == 17 && py == 11 && collected == 7) {
      points(700);
      stage++;
      if (stage > 4) {
        won = ended = 1;
      } else
        chamber();
    }
    if (turns >= 800 || !health)
      ended = 1;
  }
  if (dirty) {
    for (i = 0; i < 247; i++)
      seen[i] = pulse ? 1 : (seen[i] ? 1 : 0);
    for (y = py > 1 ? py - 2 : 0; y < 13 && y <= py + 2; y++)
      for (x = px > 1 ? px - 2 : 0; x < 19 && x <= px + 2; x++)
        seen[pos(x, y)] = 2;
  }
}

void game_draw(void) {
  uint8_t x, y, i, near;
  if (dirty) {
    screen(0, 0);
    for (y = 0; y < 13; y++)
      for (x = 0; x < 19; x++) {
        near = seen[pos(x, y)] == 2;
        tile(x, y + 3, seen[pos(x, y)] ? (maze[pos(x, y)] ? WALL : FLOOR) : 0,
             pulse ? 5 : (near ? 1 : 4));
      }
    dirty = 0;
  }
  for (i = 0; i < 3; i++)
    if (!(collected & (1 << i)) &&
        (pulse || distance(px, kx[i]) + distance(py, ky[i]) < 3))
      sprite(4 + i, 10, kx[i] * 8, ky[i] * 8 + 24, 6);
  if (pulse || seen[pos(17, 11)])
    tile(17, 14, GATE, collected == 7 ? 2 : 4);
  sprite(0, 9, px * 8, py * 8 + 24, 1);
  for (i = 0; i < 2; i++)
    if (pulse || distance(px, gx[i]) + distance(py, gy[i]) < 3)
      sprite(8 + i, 9, gx[i] * 8, gy[i] * 8 + 24, 3);
  if (decoy)
    sprite(10, 12, dx * 8, dy * 8 + 24, 6);
  hud("VAULT", stage);
  label(0, 17, pulse ? "SONAR" : "QUIET", 2);
  label(7, 17, "E", 4);
  number(8, 17, energy, 1, 2);
  label(11, 17, "KEYS", 4);
  number(16, 17,
         (collected & 1) + ((collected >> 1) & 1) + ((collected >> 2) & 1), 1,
         6);
  label(17, 17, "/3", 4);
}
