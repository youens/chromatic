#include "runtime.h"
#include "digits.h"
#include "course.h"
#include <string.h>
const char game_title[] = "COMET LINKS", game_tagline[] = "GRAVITY   GOLF",
           game_controls1[] = "D PAD AIM + POWER",
           game_controls2[] = "A PUTT  B SCOPE",
           game_goal[] = "SINK ALL 9 HOLES";
#define C RGB
const uint16_t game_palette[32] = {
    C(2, 2, 8),    C(5, 6, 14),   C(12, 13, 22), C(30, 29, 26), C(2, 2, 8),
    C(8, 14, 24),  C(16, 26, 31), C(31, 31, 31), C(2, 2, 8),    C(3, 12, 10),
    C(8, 26, 18),  C(20, 31, 24), C(2, 2, 8),    C(10, 3, 14),  C(22, 8, 24),
    C(31, 18, 28), C(2, 2, 8),    C(7, 8, 16),   C(13, 14, 24), C(20, 22, 30),
    C(2, 2, 8),    C(4, 10, 20),  C(8, 18, 28),  C(18, 28, 31), C(2, 2, 8),
    C(9, 7, 8),    C(16, 13, 12), C(24, 21, 18), C(2, 2, 8),    C(8, 5, 14),
    C(14, 9, 22),  C(29, 22, 10)};
#define SPACE 0
#define ROCK 1
#define NEBULA 2
#define BUMPER 3
#define PLANET 4
#define SUNK 1
#define LOST 2
#define STOPPED 3
#define INTRO 0
#define AIM 1
#define ROLL 2
#define DONE 3
#define PIP_HALF 92
#define PIP_FULL 94
#define PIP_EMPTY 95
uint8_t hole, strokes, shot_mode, mode_wait, aim, power, scope, scope_on, dirty;
uint8_t still, bumped, hold, dot_count, trail_i, banner_t, sunk_pal;
int8_t total;
uint16_t bx, by, rest_x, rest_y, steps;
int16_t bvx, bvy;
/* The scope's own flight, advanced a few substeps per update. */
uint16_t sim_x, sim_y, sim_steps;
int16_t sim_vx, sim_vy;
uint8_t sim_still, sim_i, sim_done;
uint8_t trail_x[3], trail_y[3], dot_x[16], dot_y[16];
/* HUD values last written; numbers cost several scanlines to redraw. */
uint8_t shown_strokes;
int8_t shown_total;
uint16_t shown_score;
const char *banner;
const uint8_t *hole_tiles;
const int8_t *hole_field;
const uint16_t row20[15] = {0,   20,  40,  60,  80,  100, 120, 140,
                            160, 180, 200, 220, 240, 260, 280};
const char *const results[] = {"ALBATROSS!", "EAGLE!", "BIRDIE!", "PAR",
                               "BOGEY",      "DOUBLE BOGEY", "TRIPLE BOGEY"};
/* Every hole has an asteroid border, so probes never leave the playfield. */
#define KIND(px, py)                                                           \
  (tile_info[hole_tiles[row20[((py) - 16) >> 3] + ((px) >> 3)]] & 15)
#define SOLID(k) ((k) == ROCK || (k) == BUMPER || (k) == PLANET)
#define DRAG(v)                                                                \
  do {                                                                         \
    if (v) {                                                                   \
      m = v > 0 ? v : -v;                                                      \
      d = heavy ? (m >> 3) + 2 : (m >> 6) + 1;                                 \
      m = m > d ? m - d : 0;                                                   \
      if (m > 1024)                                                            \
        m = 1024;                                                              \
      v = v > 0 ? (int16_t)m : -(int16_t)m;                                    \
    }                                                                          \
  } while (0)
static int16_t bounce(int16_t v, uint8_t bumper) {
  uint16_t m = v > 0 ? v : -v;
  if (bumper) {
    m = m + (m >> 2) + 64;
    if (m > 900)
      m = 900;
    bumped = 1;
  } else
    m = (m * 3) >> 2;
  return v > 0 ? -(int16_t)m : (int16_t)m;
}
/* One physics substep. Keep identical to simulate() in tools/course.py. */
static uint8_t step(void) {
  uint8_t px = bx >> 8, py = by >> 8, k, heavy, dx, dy;
  uint16_t n = row20[(py - 16) >> 3] + (px >> 3), next, m, d;
  const int8_t *f = hole_field + (n << 1);
  bvx += f[0];
  bvy += f[1];
  heavy = (tile_info[hole_tiles[n]] & 15) == NEBULA;
  DRAG(bvx);
  DRAG(bvy);
  next = bx + bvx;
  k = (uint8_t)(next >> 8) + (bvx > 0 ? 2 : bvx < 0 ? -2 : 0);
  k = KIND(k, py);
  if (SOLID(k))
    bvx = bounce(bvx, k == BUMPER);
  else {
    bx = next;
    px = bx >> 8;
  }
  next = by + bvy;
  k = (uint8_t)(next >> 8) + (bvy > 0 ? 2 : bvy < 0 ? -2 : 0);
  k = KIND(px, k);
  if (SOLID(k))
    bvy = bounce(bvy, k == BUMPER);
  else {
    by = next;
    py = by >> 8;
  }
  m = (uint16_t)(bvx < 0 ? -bvx : bvx) + (uint16_t)(bvy < 0 ? -bvy : bvy);
  dx = px > cup_x[hole] ? px - cup_x[hole] : cup_x[hole] - px;
  dy = py > cup_y[hole] ? py - cup_y[hole] : cup_y[hole] - py;
  if (dx < 4 && dy < 4 && m < 320)
    return SUNK;
  if (void_x[hole]) {
    dx = px > void_x[hole] ? px - void_x[hole] : void_x[hole] - px;
    dy = py > void_y[hole] ? py - void_y[hole] : void_y[hole] - py;
    if (dx < 4 && dy < 4)
      return LOST;
  }
  if (m < 24) {
    if (++still >= 10)
      return STOPPED;
  } else
    still = 0;
  if (++steps >= 1200)
    return STOPPED;
  return 0;
}
static void launch(void) {
  bvx = (int16_t)power * dir_x[aim];
  bvy = (int16_t)power * dir_y[aim];
  still = 0;
  steps = 0;
}
/* The scope runs the real physics forward and keeps every tenth position.
   It traces SCOPE_STEPS substeps per update so aiming never stalls. */
#define SCOPE_STEPS 5
static void trace_scope(void) {
  uint16_t ball_x = bx, ball_y = by;
  uint8_t k;
  if (!scope_on || sim_done)
    return;
  bx = sim_x, by = sim_y, bvx = sim_vx, bvy = sim_vy;
  still = sim_still, steps = sim_steps;
  for (k = 0; k < SCOPE_STEPS && !sim_done; k++) {
    if (step())
      sim_done = 1;
    if (++sim_i % 10 == 0)
      dot_x[dot_count] = bx >> 8, dot_y[dot_count++] = by >> 8;
    if (sim_i >= 160)
      sim_done = 1;
  }
  sim_x = bx, sim_y = by, sim_vx = bvx, sim_vy = bvy;
  sim_still = still, sim_steps = steps;
  bx = ball_x, by = ball_y, bvx = bvy = 0;
  bumped = 0;
}
static void predict(void) {
  uint8_t i;
  dot_count = 0;
  if (scope_on) {
    launch();
    sim_x = bx, sim_y = by, sim_vx = bvx, sim_vy = bvy;
    sim_still = sim_steps = sim_i = sim_done = 0;
    bvx = bvy = 0;
    trace_scope();
  } else
    for (i = 0; i < 4; i++) {
      int16_t k = (int16_t)(i + 1) * (4 + power);
      dot_x[i] = (bx >> 8) + (int8_t)((dir_x[aim] * k) / 48);
      dot_y[i] = (by >> 8) + (int8_t)((dir_y[aim] * k) / 48);
      dot_count++;
    }
}
static void start_hole(void) {
  hole_tiles = course_tiles + (uint16_t)hole * 300;
  hole_field = course_field + (uint16_t)hole * 600;
  bx = rest_x = (uint16_t)tee_x[hole] << 8;
  by = rest_y = (uint16_t)tee_y[hole] << 8;
  bvx = bvy = 0;
  aim = hole_aim[hole];
  power = 8;
  strokes = scope_on = 0;
  shot_mode = INTRO;
  mode_wait = 45;
  banner = hole_names[hole];
  banner_t = 45;
  memset(trail_x, 0, sizeof(trail_x));
  dirty = 1;
  predict();
}
void game_reset(void) {
  health = 0;
  hole = 0;
  total = 0;
  scope = 3;
  hold = 0;
  start_hole();
}
static void finish_hole(uint8_t picked) {
  int8_t diff = (int8_t)strokes - (int8_t)hole_par[hole];
  total += diff;
  shot_mode = DONE;
  mode_wait = 70;
  banner_t = 70;
  sunk_pal = 2;
  if (picked)
    banner = "PICKED UP", sunk_pal = 4;
  else if (strokes == 1)
    banner = "HOLE IN ONE!";
  else if (diff >= -3 && diff <= 3)
    banner = results[diff + 3];
  else
    banner = "IN THE CUP";
  if (!picked && strokes < hole_par[hole] + 3)
    points(100 * (hole_par[hole] + 3 - strokes));
  if (strokes == 1)
    points(500);
  if (!picked && diff < 0 && scope < 9)
    scope++;
  tone(picked ? 1500 : 1900, 0xA5);
}
static void aim_controls(void) {
  uint8_t old_aim = aim, old_power = power, old_scope = scope_on;
  if (keys & (J_LEFT | J_RIGHT | J_UP | J_DOWN))
    hold++;
  else
    hold = 0;
  if ((pressed & J_LEFT) || ((keys & J_LEFT) && hold > 6))
    aim = (aim + 63) & 63;
  else if ((pressed & J_RIGHT) || ((keys & J_RIGHT) && hold > 6))
    aim = (aim + 1) & 63;
  if (((pressed & J_UP) || ((keys & J_UP) && hold > 8 && !(hold % 3))) &&
      power < 16)
    power++;
  else if (((pressed & J_DOWN) || ((keys & J_DOWN) && hold > 8 && !(hold % 3))) &&
           power > 1)
    power--;
  if ((pressed & J_B) && scope && !scope_on) {
    scope--;
    scope_on = 1;
    tone(1760, 0x73);
  }
  if (aim != old_aim || power != old_power || scope_on != old_scope) {
    predict();
    if (power != old_power)
      tone(1500 + power * 20, 0x21);
  } else
    trace_scope();
  if (pressed & J_A) {
    rest_x = bx;
    rest_y = by;
    strokes++;
    launch();
    shot_mode = ROLL;
    dot_count = 0;
    noise(0x31);
    tone(1700 + power * 12, 0x82);
  }
}
static void roll(void) {
  uint8_t i, event = 0;
  for (i = 0; i < 2 && !event; i++)
    event = step();
  if (bumped) {
    bumped = 0;
    tone(1950, 0x62);
  }
  if (!(ticks & 1)) {
    trail_x[trail_i] = bx >> 8;
    trail_y[trail_i] = by >> 8;
    trail_i = trail_i == 2 ? 0 : trail_i + 1;
  }
  if (event == SUNK) {
    bx = (uint16_t)cup_x[hole] << 8;
    by = (uint16_t)cup_y[hole] << 8;
    finish_hole(0);
  } else if (event == LOST) {
    bx = rest_x;
    by = rest_y;
    noise(0x77);
    banner = "LOST IN THE VOID";
    banner_t = 60;
    memset(trail_x, 0, sizeof(trail_x));
  }
  if (event == STOPPED || event == LOST) {
    bvx = bvy = 0;
    if (strokes >= hole_par[hole] + 3) {
      strokes = hole_par[hole] + 4;
      finish_hole(1);
      return;
    }
    shot_mode = AIM;
    scope_on = 0;
    predict();
  }
}
void game_update(void) {
  if (banner_t)
    banner_t--;
  if (shot_mode == INTRO) {
    if (!--mode_wait || (pressed & J_A))
      shot_mode = AIM, banner_t = 0;
  } else if (shot_mode == AIM)
    aim_controls();
  else if (shot_mode == ROLL)
    roll();
  else if (!--mode_wait) {
    if (++hole == HOLE_COUNT) {
      points(scope * 100);
      ended = won = 1;
    } else
      start_hole();
  }
}
static void signed_total(uint8_t x, uint8_t y) {
  uint8_t n = total < 0 ? -total : total, pal = total > 0 ? 3 : 2;
  if (!total)
    label(x + 2, y, "E", 2);
  else if (n < 10) {
    label(x + 1, y, total > 0 ? "+" : "-", pal);
    digits(x + 2, y, n, 1, pal);
  } else {
    label(x, y, total > 0 ? "+" : "-", pal);
    digits(x + 1, y, n, 2, pal);
  }
}
void game_draw(void) {
  uint8_t x, y, i, t;
  if (dirty) {
    backdrop();
    for (y = 0; y < 15; y++)
      for (x = 0; x < 20; x++) {
        t = hole_tiles[row20[y] + x];
        if (t)
          tile(x, y + 2, t, tile_info[t] >> 4);
      }
    label(0, 0, "HOLE", 4);
    tile(5, 0, 17 + hole + 1, 1);
    label(7, 0, "PAR", 4);
    tile(11, 0, 17 + hole_par[hole], 1);
    label(13, 0, "SHOT", 4);
    label(11, 1, "TOTAL", 4);
    shown_strokes = shown_total = 99;
    shown_score = score + 1;
    dirty = 0;
  }
  if (strokes != shown_strokes) {
    digits(18, 0, strokes, 2, 1);
    shown_strokes = strokes;
  }
  if (score != shown_score) {
    digits(0, 1, score, 5, 1);
    shown_score = score;
  }
  if (total != shown_total) {
    memset(tiles + 49, 0, 3);
    signed_total(17, 1);
    shown_total = total;
  }
  if (shot_mode != DONE || (ticks & 4))
    sprite(0, 16, (bx >> 8) - 4, (by >> 8) - 4, 1);
  if (shot_mode == ROLL)
    for (i = 0; i < 3; i++)
      if (trail_x[i])
        sprite(1 + i, i == trail_i ? 18 : 17, trail_x[i] - 4, trail_y[i] - 4,
               1);
  if (shot_mode == AIM)
    for (i = 0; i < dot_count; i++)
      sprite(4 + i, scope_on ? 20 : 19, dot_x[i] - 4, dot_y[i] - 4,
             scope_on ? 2 : 4);
  sprite(20, 21 + ((ticks >> 3) & 1), cup_x[hole] - 1, cup_y[hole] - 12, 2);
  if (void_x[hole])
    sprite(21, 23 + ((ticks >> 2) & 1), void_x[hole] - 4, void_y[hole] - 4, 3);
  if (shot_mode == DONE && sunk_pal == 2)
    for (i = 0; i < 4; i++)
      sprite(22 + i, 25, cup_x[hole] - 4 + ((i & 1) ? 1 : -1) * (70 - mode_wait) / 3,
             cup_y[hole] - 4 + ((i & 2) ? 1 : -1) * (70 - mode_wait) / 3, 2);
  if (banner_t)
    label(0, 17, banner, shot_mode == DONE ? sunk_pal : 1);
  else {
    label(0, 17, "PWR", 4);
    for (i = 0; i < 8; i++)
      tile(4 + i, 17,
           power >= i * 2 + 2 ? PIP_FULL : power == i * 2 + 1 ? PIP_HALF
                                                              : PIP_EMPTY,
           power > 12 ? 3 : 1);
    label(13, 17, "SCOPE", scope_on ? 2 : 4);
    tile(19, 17, 17 + scope, scope ? 2 : 4);
  }
}
