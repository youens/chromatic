#include "runtime.h"
#include "digits.h"
#include "sky.h"
#include <string.h>
const char game_title[] = "STORMKITE", game_tagline[] = "RIDE THE  THUNDER",
           game_controls1[] = "D PAD FLY THE KITE",
           game_controls2[] = "A DARTS  B GUST",
           game_goal[] = "BREAK THE STORM";
#define C RGB
const uint16_t game_palette[32] = {
    C(3, 3, 10),  C(5, 5, 14),   C(12, 12, 22), C(30, 29, 26), C(3, 3, 10),
    C(18, 4, 8),  C(31, 12, 10), C(31, 28, 22), C(3, 3, 10),   C(3, 10, 14),
    C(6, 20, 22), C(16, 31, 28), C(3, 3, 10),   C(10, 6, 20),  C(20, 14, 31),
    C(31, 31, 14), C(3, 3, 10),  C(8, 8, 18),   C(16, 16, 26), C(24, 24, 30),
    C(3, 3, 10),  C(9, 6, 18),   C(21, 10, 19), C(30, 24, 26), C(3, 3, 10),
    C(10, 4, 12), C(26, 11, 9),  C(31, 24, 8),  C(3, 3, 10),   C(4, 5, 13),
    C(7, 14, 20), C(29, 22, 9)};
#define DARTS 4
#define BOLTS 5
#define FOES 5
#define ID_TAIL 4
#define ID_DART 5
#define ID_BOLT 9
#define ID_LANTERN 14
#define ID_POP 15
#define ID_FOE 17
#define ID_BOSS 25
#define GUST_FULL 24
#define BOSS_HP 48
#define WISP 1
#define SQUALL 2
#define SWIFT 3
uint8_t kx, ky, fire_wait, invincible, gust, wave, banner, dirty;
uint8_t wind_warn, wind_time;
int8_t wind, wind_next;
uint8_t dart_x[DARTS], dart_y[DARTS];
uint8_t bolt_x[BOLTS], bolt_y[BOLTS];
int8_t bolt_dy[BOLTS];
uint8_t foe[FOES], foe_x[FOES], foe_y[FOES], foe_hp[FOES], foe_t[FOES],
    foe_home[FOES];
uint8_t lantern_x, lantern_y, lantern_t;
uint8_t pop_x[2], pop_y[2], pop_t[2], pop_next;
uint8_t boss, boss_hp, boss_x, boss_y, boss_t, boss_hurt;
uint16_t kills, shown_score;
uint8_t shown_wave, shown_health, shown_boss;
/* Spawner countdowns; each reloads with its period for the current wave. */
uint8_t wait_wisp, wait_flock, wait_squall, wait_swift, wait_wind, wait_thunder,
    wait_lantern;
const uint16_t wave_at[4] = {1, 600, 1350, 2100};
const char *const wave_names[4] = {"WAVE 1 DUSK", "WAVE 2 NIGHTFALL",
                                   "WAVE 3 SQUALL LINE", "WAVE 4 THUNDERHEAD"};
/* Periods per wave (0 = off) and the delay before each first appearance. */
const uint8_t every_wisp[5] = {0, 28, 32, 36, 0}, every_flock[5] = {0, 140, 0, 0, 0},
              every_squall[5] = {0, 0, 150, 120, 0}, every_swift[5] = {0, 0, 0, 50, 0},
              every_wind[5] = {0, 0, 0, 240, 0}, every_thunder[5] = {0, 0, 0, 170, 170},
              every_lantern[5] = {0, 180, 180, 180, 150};
#define NEAR(a, b, r) ((uint8_t)((a) > (b) ? (a) - (b) : (b) - (a)) < (r))
#define SIZE(kind) ((kind) == SQUALL ? 16 : 8)
const int8_t wobble[32] = {0,  2,   4,   6,   7,   8,   9,  10, 10, 10, 9,
                           8,  7,   6,   4,   2,   0,   -2, -4, -6, -7, -8,
                           -9, -10, -10, -10, -9,  -8,  -7, -6, -4, -2};
static void pop(uint8_t x, uint8_t y) {
  pop_x[pop_next] = x;
  pop_y[pop_next] = y;
  pop_t[pop_next] = 8;
  pop_next ^= 1;
}
static void spawn(uint8_t kind, uint8_t x, uint8_t y) {
  uint8_t i, limit = boss ? 2 : FOES;
  for (i = 0; i < limit; i++)
    if (!foe[i]) {
      foe[i] = kind;
      foe_x[i] = x;
      foe_y[i] = foe_home[i] = y;
      foe_hp[i] = kind == SQUALL ? 3 : 1;
      foe_t[i] = (uint8_t)random16();
      if (kind == SQUALL)
        foe_t[i] = 0;
      return;
    }
}
static void bolt(uint8_t x, uint8_t y, int8_t dy) {
  uint8_t i;
  for (i = 0; i < BOLTS; i++)
    if (!bolt_x[i]) {
      bolt_x[i] = x;
      bolt_y[i] = y;
      bolt_dy[i] = dy;
      return;
    }
}
static int8_t aim(uint8_t y) {
  int16_t d = (int16_t)ky + 4 - y;
  if (d > 56)
    return 2;
  if (d > 20)
    return 1;
  if (d < -56)
    return -2;
  if (d < -20)
    return -1;
  return 0;
}
static void charge(uint8_t n) {
  gust = gust + n > GUST_FULL ? GUST_FULL : gust + n;
}
static void defeat(uint8_t i) {
  uint8_t kind = foe[i];
  foe[i] = 0;
  kills++;
  pop(foe_x[i] + (kind == SQUALL ? 4 : 0), foe_y[i] + (kind == SQUALL ? 4 : 0));
  points(kind == SQUALL ? 150 : kind == SWIFT ? 80 : 50);
  charge(kind == SQUALL ? 3 : kind == SWIFT ? 2 : 1);
  tone(kind == SQUALL ? 1880 : 1820, 0x72);
}
static void hurt(void) {
  if (invincible)
    return;
  health--;
  invincible = 60;
  noise(0x67);
}
void game_reset(void) {
  health = 5;
  kx = 24;
  ky = 64;
  fire_wait = invincible = gust = wave = banner = boss = 0;
  wind = wind_next = wind_warn = wind_time = 0;
  lantern_x = lantern_t = pop_next = wait_lantern = 0;
  wait_wisp = wait_flock = wait_squall = wait_swift = wait_wind = wait_thunder = 0;

  kills = 0;
  memset(dart_x, 0, sizeof(dart_x));
  memset(bolt_x, 0, sizeof(bolt_x));
  memset(foe, 0, sizeof(foe));
  memset(pop_t, 0, sizeof(pop_t));
  sky_mood = 0;
  sky_speed = 4;
  dirty = 1;
  sky_start();
}
static uint8_t due(uint8_t *wait, const uint8_t *every) {
  if (!*wait || --*wait)
    return 0;
  *wait = every[wave];
  return 1;
}
static uint8_t altitude(void) { return 30 + (uint8_t)random16() % 80; }
static void begin_wave(void) {
  wave++;
  banner = 75;
  sky_mood = wave == 1 ? 0 : wave == 2 ? 1 : 2;
  sky_speed = wave >= 3 ? 6 : 4;
  tone(1700 + wave * 60, 0x94);
  wait_wisp = every_wisp[wave];
  wait_flock = every_flock[wave] ? 70 : 0;
  wait_squall = every_squall[wave] ? (wave == 2 ? 20 : 60) : 0;
  wait_swift = every_swift[wave] ? 25 : 0;
  wait_wind = every_wind[wave] ? 100 : 0;
  wait_thunder = every_thunder[wave];
  if (!wait_lantern)
    wait_lantern = 90;
  if (wave == 4) {
    boss = 1;
    boss_x = 168;
    boss_y = 48;
    boss_hp = BOSS_HP;
    boss_t = boss_hurt = 0;
  }
}
static void schedule(void) {
  uint8_t i;
  if (wave < 4 && ticks >= wave_at[wave])
    begin_wave();
  if (due(&wait_wisp, every_wisp))
    spawn(WISP, 168, altitude());
  if (due(&wait_flock, every_flock))
    for (i = 0; i < 3; i++)
      spawn(WISP, 168 + i * 12, 34 + i * 30);
  if (due(&wait_squall, every_squall))
    spawn(SQUALL, 168, 24 + (uint8_t)random16() % 80);
  if (due(&wait_swift, every_swift))
    spawn(SWIFT, 168, altitude());
  if (due(&wait_wind, every_wind)) {
    wind_next = (random16() & 1) ? 1 : -1;
    wind_warn = 30;
  }
  if (due(&wait_thunder, every_thunder)) {
    sky_flash = 6;
    noise(0x77);
  }
  if (wait_lantern && !--wait_lantern) {
    wait_lantern = every_lantern[wave];
    if (!lantern_x) {
      lantern_x = 168;
      lantern_y = 28 + (uint8_t)random16() % 84;
      lantern_t = 0;
    }
  }
}
static void move_kite(void) {
  if ((keys & J_LEFT) && kx > 2)
    kx -= 2;
  if ((keys & J_RIGHT) && kx < 140)
    kx += 2;
  if ((keys & J_UP) && ky > 18)
    ky -= 2;
  if ((keys & J_DOWN) && ky < 118)
    ky += 2;
  if (wind_warn && !--wind_warn) {
    wind = wind_next;
    wind_time = 75;
  }
  if (wind_time && !--wind_time)
    wind = 0;
  if (wind < 0 && ky > 18)
    ky--;
  if (wind > 0 && ky < 118)
    ky++;
}
static void release_gust(void) {
  uint8_t i;
  gust = 0;
  if (invincible < 30)
    invincible = 30;
  sky_flash = 10;
  noise(0x46);
  tone(1600, 0xB7);
  memset(bolt_x, 0, sizeof(bolt_x));
  for (i = 0; i < FOES; i++)
    if (foe[i]) {
      if (foe_hp[i] > 2)
        foe_hp[i] -= 2;
      else
        defeat(i);
    }
  if (boss == 1) {
    boss_hp = boss_hp > 8 ? boss_hp - 8 : 0;
    boss_hurt = 8;
    points(80);
  }
}
static void update_darts(void) {
  uint8_t i, j, s, cx, cy;
  if (fire_wait)
    fire_wait--;
  if ((keys & J_A) && !fire_wait)
    for (i = 0; i < DARTS; i++)
      if (!dart_x[i]) {
        dart_x[i] = kx + 12;
        dart_y[i] = ky + 4;
        fire_wait = 4;
        if (!(i & 1))
          tone(1985, 0x21);
        break;
      }
  for (i = 0; i < DARTS; i++) {
    if (!dart_x[i])
      continue;
    dart_x[i] += 6;
    if (dart_x[i] > 160) {
      dart_x[i] = 0;
      continue;
    }
    cx = dart_x[i] + 4;
    cy = dart_y[i] + 4;
    for (j = 0; j < FOES; j++) {
      if (!foe[j])
        continue;
      s = SIZE(foe[j]) >> 1;
      if (NEAR(cx, foe_x[j] + s, s + 3) &&
          NEAR(cy, foe_y[j] + s, s + 2)) {
        dart_x[i] = 0;
        if (!--foe_hp[j])
          defeat(j);
        else
          noise(0x2A);
        break;
      }
    }
    if (dart_x[i] && boss == 1 && NEAR(cx, boss_x + 12, 14) &&
        NEAR(cy, boss_y + 12, 13)) {
      dart_x[i] = 0;
      if (boss_hp)
        boss_hp--;
      boss_hurt = 3;
      points(10);
      if (!(boss_hp & 3))
        noise(0x15);
    }
  }
}
static void update_foes(void) {
  uint8_t i, s, fast = wave >= 3 ? 2 : 1;
  for (i = 0; i < FOES; i++) {
    if (!foe[i])
      continue;
    foe_t[i]++;
    if (foe[i] == WISP) {
      foe_x[i] -= fast;
      foe_y[i] = foe_home[i] + wobble[foe_t[i] & 31];
    } else if (foe[i] == SWIFT) {
      foe_x[i] -= 3;
      if (foe_x[i] > kx + 16) {
        if (foe_y[i] < ky + 4)
          foe_y[i]++;
        else if (foe_y[i] > ky + 4)
          foe_y[i]--;
      }
    } else {
      if (foe_t[i] > 170)
        foe_x[i] -= 3;
      else if (foe_x[i] > 112 + (i & 1) * 20)
        foe_x[i] -= 2;
      else if (foe_t[i] % 40 == 0)
        bolt(foe_x[i], foe_y[i] + 4, aim(foe_y[i] + 4));
      foe_y[i] = foe_home[i] + (wobble[(foe_t[i] >> 1) & 31] >> 1);
    }
    if (foe_x[i] < 4 || foe_x[i] > 200) {
      foe[i] = 0;
      continue;
    }
    s = SIZE(foe[i]) >> 1;
    if (NEAR(kx + 8, foe_x[i] + s, s + 4) &&
        NEAR(ky + 7, foe_y[i] + s, s + 3)) {
      hurt();
      if (foe[i] != SQUALL)
        defeat(i);
    }
  }
}
static void update_bolts(void) {
  uint8_t i;
  for (i = 0; i < BOLTS; i++) {
    if (!bolt_x[i])
      continue;
    bolt_x[i] -= 3;
    bolt_y[i] += bolt_dy[i];
    if (bolt_x[i] < 4 || bolt_x[i] > 200 || bolt_y[i] < 16 ||
        bolt_y[i] > 132) {
      bolt_x[i] = 0;
      continue;
    }
    if (NEAR(bolt_x[i] + 4, kx + 8, 6) &&
        NEAR(bolt_y[i] + 4, ky + 7, 6)) {
      bolt_x[i] = 0;
      hurt();
    }
  }
}
static void update_boss(void) {
  uint8_t rate;
  if (boss_hurt)
    boss_hurt--;
  boss_t++;
  if (boss == 2) {
    if (!(boss_t & 7))
      pop(boss_x + (boss_t & 15), boss_y + ((boss_t >> 1) & 15));
    if (!(boss_t & 15))
      noise(0x57);
    if (boss_t >= 64)
      ended = won = 1;
    return;
  }
  if (boss_x > 120)
    boss_x--;
  boss_y = 52 + wobble[(boss_t >> 1) & 31] * 3;
  if (boss_x <= 120) {
    rate = boss_hp > BOSS_HP / 2 ? 44 : 30;
    if (boss_t % rate == 0) {
      bolt(boss_x, boss_y + 8, -1);
      bolt(boss_x, boss_y + 10, 0);
      bolt(boss_x, boss_y + 12, 1);
      if (boss_hp <= BOSS_HP / 2) {
        bolt(boss_x + 4, boss_y + 6, -2);
        bolt(boss_x + 4, boss_y + 14, 2);
      }
      tone(1500, 0x83);
    }
    if (boss_t % 160 == 80) {
      spawn(WISP, boss_x - 8, boss_y > 40 ? boss_y - 16 : 24);
      spawn(WISP, boss_x - 8, boss_y < 80 ? boss_y + 32 : 112);
    }
  }
  if (NEAR(kx + 8, boss_x + 12, 16) && NEAR(ky + 7, boss_y + 12, 15))
    hurt();
  if (!boss_hp) {
    boss = 2;
    boss_t = 0;
    points(2000 + health * 300);
    memset(bolt_x, 0, sizeof(bolt_x));
    memset(foe, 0, sizeof(foe));
    tone(1960, 0xA7);
  }
}
void game_update(void) {
  uint8_t i;
  if (invincible)
    invincible--;
  if (banner)
    banner--;
  for (i = 0; i < 2; i++)
    if (pop_t[i])
      pop_t[i]--;
  schedule();
  move_kite();
  if ((pressed & J_B) && gust >= GUST_FULL && boss != 2)
    release_gust();
  update_darts();
  update_foes();
  update_bolts();
  if (boss)
    update_boss();
  if (lantern_x) {
    lantern_x--;
    lantern_t++;
    if (lantern_x < 4)
      lantern_x = 0;
    else if (NEAR(lantern_x + 4, kx + 8, 10) &&
             NEAR(lantern_y + 4 + (wobble[lantern_t & 31] >> 2), ky + 8, 11)) {
      lantern_x = 0;
      charge(8);
      points(100);
      tone(1950, 0x94);
    }
  }
  if (!health)
    ended = 1;
}
void game_draw(void) {
  uint8_t i, k;
  if (dirty) {
    backdrop();
    label(0, 0, "SCORE", 4);
    label(13, 0, "WAVE", 4);
    shown_score = score + 1;
    shown_wave = shown_health = shown_boss = 255;
    dirty = 0;
  }
  if (boss != 2 || (ticks & 4)) {
    if (!invincible || (ticks & 2)) {
      actor(0, 0, kx, ky, 1, 0);
      sprite(ID_TAIL, 21, kx - 5, ky + 13 + (wobble[(ticks << 1) & 31] >> 2),
             1);
    }
  }
  for (i = 0; i < DARTS; i++)
    if (dart_x[i])
      sprite(ID_DART + i, 16, dart_x[i], dart_y[i], 1);
  for (i = 0; i < BOLTS; i++)
    if (bolt_x[i])
      sprite(ID_BOLT + i, 19, bolt_x[i], bolt_y[i], 3);
  if (lantern_x)
    sprite(ID_LANTERN, 20, lantern_x,
           lantern_y + (wobble[lantern_t & 31] >> 2), 6);
  for (i = 0; i < 2; i++)
    if (pop_t[i])
      sprite(ID_POP + i, pop_t[i] > 4 ? 25 : 8, pop_x[i], pop_y[i], 3);
  for (i = 0; i < FOES; i++) {
    if (foe[i] == WISP)
      sprite(ID_FOE + i * 4, 17 + ((ticks >> 2) & 1), foe_x[i], foe_y[i], 3);
    else if (foe[i] == SWIFT)
      sprite(ID_FOE + i * 4, 22 + ((ticks >> 1) & 1), foe_x[i], foe_y[i], 3);
    else if (foe[i] == SQUALL)
      actor(ID_FOE + i * 4, 4, foe_x[i], foe_y[i], foe_hp[i] < 3 ? 1 : 3, 0);
  }
  if (boss && (boss == 1 || (ticks & 2))) {
    uint8_t row, col, pal = boss_hurt ? 1 : 3, id = ID_BOSS;
    for (row = 0; row < 3; row++)
      for (col = 0; col < 3; col++, id++) {
        uint8_t x = boss_x + col * 8;
        set_sprite_tile(id, 26 + row * 3 + col);
        set_sprite_prop(id, pal);
        move_sprite(id, x < 160 ? x + 8 : 0, boss_y + row * 8 + 16);
      }
  }
  /* Cached HUD: the runtime hud() redraws nine digits every update. */
  if (score != shown_score) {
    digits(0, 1, score, 5, 1);
    shown_score = score;
  }
  if (wave != shown_wave) {
    tile(18, 1, 17 + wave, 2);
    shown_wave = wave;
  }
  if (health != shown_health) {
    for (i = 0; i < 6; i++)
      tile(7 + i, 0, i < health ? HEART : 0, 3);
    shown_health = health;
  }
  k = boss == 1 ? (boss_hp + 5) / 6 : 0;
  if (k != shown_boss) {
    for (i = 0; i < 8; i++)
      tile(6 + i, 1, boss == 1 ? (i < k ? LINE : VOID) : 0, 3);
    shown_boss = k;
  }
  if (banner)
    label(1, 17, wave_names[wave - 1], 1);
  else if (wind_warn || wind) {
    label(0, 17, (wind_next < 0) ? "WIND RISING" : "DOWNDRAFT", 3);
  } else {
    label(0, 17, "GUST", 2);
    for (i = 0; i < 8; i++)
      tile(5 + i, 17, i * 3 < gust ? LINE : VOID, 2);
    if (gust >= GUST_FULL && (ticks & 8))
      label(14, 17, "B GO!", 1);
  }
}
