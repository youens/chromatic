/* Rival behaviour: wave scripts, flight paths, aimed fire and the four
   bosses' attack patterns. Shares a bank with the fixed helpers. */
#ifndef ARCADE
#pragma bank 5
#endif
#include "dotwing.h"
#include <string.h>

enum { K_DRONE, K_GUNNER, K_HEAVY };
enum { M_DIVE, M_SWOOP_L, M_SWOOP_R, M_SNAKE, M_HOVER, M_SIDE_L, M_SIDE_R };
enum { B_SMALL, B_BIG, B_NEEDLE, B_STAR };

static const int8_t sin64[64] = {0,6,12,18,24,30,35,40,45,49,52,56,58,60,62,63,63,63,62,60,58,56,52,49,45,40,35,30,24,18,12,6,0,-6,-12,-18,-24,-30,-35,-40,-45,-49,-52,-56,-58,-60,-62,-63,-63,-63,-62,-60,-58,-56,-52,-49,-45,-40,-35,-30,-24,-18,-12,-6};
static const int8_t dir_x[32] = {64,63,59,53,45,36,24,12,0,-12,-24,-36,-45,-53,-59,-63,-64,-63,-59,-53,-45,-36,-24,-12,0,12,24,36,45,53,59,63};
static const int8_t dir_y[32] = {0,12,24,36,45,53,59,63,64,63,59,53,45,36,24,12,0,-12,-24,-36,-45,-53,-59,-63,-64,-63,-59,-53,-45,-36,-24,-12};
/* sin64 * 64: a 16-pixel swing in 8.8 fixed point. */
static const int16_t wave16[64] = {0,384,768,1152,1536,1920,2240,2560,2880,3136,3328,3584,3712,3840,3968,4032,4032,4032,3968,3840,3712,3584,3328,3136,2880,2560,2240,1920,1536,1152,768,384,0,-384,-768,-1152,-1536,-1920,-2240,-2560,-2880,-3136,-3328,-3584,-3712,-3840,-3968,-4032,-4032,-4032,-3968,-3840,-3712,-3584,-3328,-3136,-2880,-2560,-2240,-1920,-1536,-1152,-768,-384};
static const uint8_t atan_tab[17] = {0,0,1,1,1,2,2,2,2,3,3,3,3,3,4,4,4};

/* Bullet velocities (8.8) by speed class and direction; no multiplies. */
static const uint8_t speed_class[40] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,6,7,7,7,7,7,7,7,7,7};
static const int16_t vel_x[8][32] = {
  {256,252,236,212,180,144,96,48,0,-48,-96,-144,-180,-212,-236,-252,-256,-252,-236,-212,-180,-144,-96,-48,0,48,96,144,180,212,236,252},
  {288,283,265,238,202,162,108,54,0,-54,-108,-162,-203,-239,-266,-284,-288,-284,-266,-239,-203,-162,-108,-54,0,54,108,162,202,238,265,283},
  {320,315,295,265,225,180,120,60,0,-60,-120,-180,-225,-265,-295,-315,-320,-315,-295,-265,-225,-180,-120,-60,0,60,120,180,225,265,295,315},
  {352,346,324,291,247,198,132,66,0,-66,-132,-198,-248,-292,-325,-347,-352,-347,-325,-292,-248,-198,-132,-66,0,66,132,198,247,291,324,346},
  {384,378,354,318,270,216,144,72,0,-72,-144,-216,-270,-318,-354,-378,-384,-378,-354,-318,-270,-216,-144,-72,0,72,144,216,270,318,354,378},
  {416,409,383,344,292,234,156,78,0,-78,-156,-234,-293,-345,-384,-410,-416,-410,-384,-345,-293,-234,-156,-78,0,78,156,234,292,344,383,409},
  {448,441,413,371,315,252,168,84,0,-84,-168,-252,-315,-371,-413,-441,-448,-441,-413,-371,-315,-252,-168,-84,0,84,168,252,315,371,413,441},
  {512,504,472,424,360,288,192,96,0,-96,-192,-288,-360,-424,-472,-504,-512,-504,-472,-424,-360,-288,-192,-96,0,96,192,288,360,424,472,504}};
static const int16_t vel_y[8][32] = {
  {0,48,96,144,180,212,236,252,256,252,236,212,180,144,96,48,0,-48,-96,-144,-180,-212,-236,-252,-256,-252,-236,-212,-180,-144,-96,-48},
  {0,54,108,162,202,238,265,283,288,283,265,238,202,162,108,54,0,-54,-108,-162,-203,-239,-266,-284,-288,-284,-266,-239,-203,-162,-108,-54},
  {0,60,120,180,225,265,295,315,320,315,295,265,225,180,120,60,0,-60,-120,-180,-225,-265,-295,-315,-320,-315,-295,-265,-225,-180,-120,-60},
  {0,66,132,198,247,291,324,346,352,346,324,291,247,198,132,66,0,-66,-132,-198,-248,-292,-325,-347,-352,-347,-325,-292,-248,-198,-132,-66},
  {0,72,144,216,270,318,354,378,384,378,354,318,270,216,144,72,0,-72,-144,-216,-270,-318,-354,-378,-384,-378,-354,-318,-270,-216,-144,-72},
  {0,78,156,234,292,344,383,409,416,409,383,344,292,234,156,78,0,-78,-156,-234,-293,-345,-384,-410,-416,-410,-384,-345,-293,-234,-156,-78},
  {0,84,168,252,315,371,413,441,448,441,413,371,315,252,168,84,0,-84,-168,-252,-315,-371,-413,-441,-448,-441,-413,-371,-315,-252,-168,-84},
  {0,96,192,288,360,424,472,504,512,504,472,424,360,288,192,96,0,-96,-192,-288,-360,-424,-472,-504,-512,-504,-472,-424,-360,-288,-192,-96}};

#define SECTOR_IDX ((uint8_t)(dw_sector - 1u) & 3u)
uint8_t dw_boss_t;
static uint8_t wave_i, boss_phase, boss_fire, boss_step, boss_spin;
static uint16_t boss_third, boss_two_thirds;

static uint8_t gap(int16_t a, int16_t b) {
  int16_t d = a - b;
  if (d < 0) d = -d;
  return d > 255 ? 255 : (uint8_t)d;
}

/* --------------------------------------------------------- wave scripts */
/* at (in 16-pixel steps of scroll), kind, mode, x, count, gap, x step */
typedef struct { uint8_t at, kind, mode, x, count, gap; int8_t dx; } Wave;
#define END {255, 0, 0, 0, 0, 0, 0}
static const Wave wave_grok[] = {
  {6, K_DRONE, M_DIVE, 28, 4, 18, 0}, {9, K_DRONE, M_DIVE, 116, 4, 18, 0},
  {13, K_DRONE, M_SWOOP_L, 0, 5, 12, 0}, {17, K_DRONE, M_SWOOP_R, 0, 5, 12, 0},
  {21, K_GUNNER, M_HOVER, 72, 1, 0, 0}, {25, K_DRONE, M_SNAKE, 36, 6, 10, 0},
  {28, K_DRONE, M_SNAKE, 108, 6, 10, 0}, {32, K_GUNNER, M_HOVER, 32, 2, 30, 80},
  {37, K_DRONE, M_DIVE, 16, 6, 14, 22}, {41, K_DRONE, M_SIDE_L, 20, 4, 16, 0},
  {44, K_DRONE, M_SIDE_R, 44, 4, 16, 0}, {48, K_HEAVY, M_DIVE, 64, 1, 0, 0},
  {54, K_DRONE, M_SWOOP_L, 0, 6, 10, 0}, {56, K_DRONE, M_SWOOP_R, 0, 6, 10, 0},
  {61, K_GUNNER, M_HOVER, 20, 3, 24, 52}, {67, K_DRONE, M_SNAKE, 72, 8, 8, 0},
  {72, K_DRONE, M_DIVE, 136, 6, 14, -22}, {76, K_GUNNER, M_DIVE, 40, 2, 40, 64},
  {80, K_DRONE, M_SIDE_L, 16, 5, 12, 0}, {81, K_DRONE, M_SIDE_R, 52, 5, 12, 0},
  {87, K_HEAVY, M_HOVER, 64, 1, 0, 0}, {93, K_DRONE, M_SWOOP_L, 0, 6, 10, 0},
  {95, K_DRONE, M_SWOOP_R, 0, 6, 10, 0}, {100, K_GUNNER, M_HOVER, 24, 2, 20, 96},
  {105, K_DRONE, M_SNAKE, 40, 8, 8, 0}, {107, K_DRONE, M_SNAKE, 104, 8, 8, 0},
  {113, K_HEAVY, M_DIVE, 24, 2, 90, 80}, {119, K_GUNNER, M_HOVER, 16, 3, 30, 56},
  {125, K_DRONE, M_DIVE, 16, 8, 10, 16}, {131, K_DRONE, M_SWOOP_L, 0, 8, 8, 0},
  {133, K_DRONE, M_SWOOP_R, 0, 8, 8, 0}, {139, K_GUNNER, M_HOVER, 40, 3, 20, 40},
  END};
static const Wave wave_claude[] = {
  {6, K_DRONE, M_SWOOP_L, 0, 6, 10, 0}, {9, K_DRONE, M_SWOOP_R, 0, 6, 10, 0},
  {13, K_GUNNER, M_HOVER, 40, 2, 24, 64}, {18, K_DRONE, M_SNAKE, 72, 8, 8, 0},
  {22, K_DRONE, M_DIVE, 16, 6, 12, 24}, {25, K_DRONE, M_DIVE, 136, 6, 12, -24},
  {29, K_HEAVY, M_DIVE, 64, 1, 0, 0}, {33, K_GUNNER, M_SIDE_L, 28, 2, 40, 0},
  {35, K_GUNNER, M_SIDE_R, 52, 2, 40, 0}, {40, K_DRONE, M_SWOOP_L, 0, 8, 8, 0},
  {42, K_DRONE, M_SWOOP_R, 0, 8, 8, 0}, {47, K_GUNNER, M_HOVER, 16, 4, 20, 40},
  {53, K_DRONE, M_SNAKE, 32, 8, 8, 0}, {55, K_DRONE, M_SNAKE, 112, 8, 8, 0},
  {60, K_HEAVY, M_HOVER, 32, 2, 50, 64}, {67, K_DRONE, M_SIDE_L, 16, 6, 10, 0},
  {69, K_DRONE, M_SIDE_R, 40, 6, 10, 0}, {74, K_GUNNER, M_DIVE, 24, 4, 24, 36},
  {80, K_DRONE, M_DIVE, 72, 10, 8, 0}, {85, K_GUNNER, M_HOVER, 72, 1, 0, 0},
  {86, K_DRONE, M_SWOOP_L, 0, 6, 10, 0}, {88, K_DRONE, M_SWOOP_R, 0, 6, 10, 0},
  {94, K_HEAVY, M_DIVE, 16, 3, 60, 48}, {101, K_GUNNER, M_HOVER, 24, 3, 20, 48},
  {107, K_DRONE, M_SNAKE, 56, 10, 7, 0}, {111, K_DRONE, M_DIVE, 16, 8, 10, 18},
  {116, K_GUNNER, M_SIDE_L, 24, 3, 30, 0}, {118, K_GUNNER, M_SIDE_R, 48, 3, 30, 0},
  {124, K_HEAVY, M_HOVER, 64, 1, 0, 0}, {126, K_DRONE, M_SWOOP_L, 0, 8, 8, 0},
  {128, K_DRONE, M_SWOOP_R, 0, 8, 8, 0}, {135, K_GUNNER, M_HOVER, 16, 4, 16, 40},
  END};
static const Wave wave_gemini[] = {
  {6, K_DRONE, M_SNAKE, 40, 8, 8, 0}, {7, K_DRONE, M_SNAKE, 104, 8, 8, 0},
  {12, K_GUNNER, M_HOVER, 24, 2, 0, 96}, {16, K_DRONE, M_SWOOP_L, 0, 8, 8, 0},
  {18, K_DRONE, M_SWOOP_R, 0, 8, 8, 0}, {23, K_HEAVY, M_DIVE, 64, 1, 0, 0},
  {26, K_GUNNER, M_SIDE_L, 20, 3, 30, 0}, {28, K_GUNNER, M_SIDE_R, 44, 3, 30, 0},
  {33, K_DRONE, M_DIVE, 16, 10, 8, 14}, {37, K_GUNNER, M_HOVER, 16, 4, 16, 40},
  {43, K_DRONE, M_SNAKE, 72, 10, 7, 0}, {47, K_HEAVY, M_HOVER, 24, 2, 0, 80},
  {54, K_DRONE, M_SWOOP_L, 0, 8, 8, 0}, {55, K_DRONE, M_SWOOP_R, 0, 8, 8, 0},
  {60, K_GUNNER, M_DIVE, 24, 4, 20, 32}, {65, K_DRONE, M_SIDE_L, 12, 6, 10, 0},
  {66, K_DRONE, M_SIDE_R, 36, 6, 10, 0}, {71, K_HEAVY, M_DIVE, 40, 2, 70, 48},
  {77, K_GUNNER, M_HOVER, 72, 1, 0, 0}, {78, K_DRONE, M_SNAKE, 24, 8, 8, 0},
  {80, K_DRONE, M_SNAKE, 120, 8, 8, 0}, {86, K_GUNNER, M_HOVER, 16, 4, 14, 40},
  {92, K_DRONE, M_DIVE, 136, 10, 8, -14}, {97, K_HEAVY, M_HOVER, 64, 1, 0, 0},
  {99, K_DRONE, M_SWOOP_L, 0, 8, 8, 0}, {101, K_DRONE, M_SWOOP_R, 0, 8, 8, 0},
  {107, K_GUNNER, M_SIDE_L, 16, 4, 24, 0}, {109, K_GUNNER, M_SIDE_R, 40, 4, 24, 0},
  {116, K_HEAVY, M_DIVE, 16, 3, 50, 48}, {123, K_DRONE, M_SNAKE, 72, 12, 6, 0},
  {128, K_GUNNER, M_HOVER, 24, 3, 16, 48}, {134, K_DRONE, M_SWOOP_L, 0, 10, 7, 0},
  {135, K_DRONE, M_SWOOP_R, 0, 10, 7, 0}, END};
static const Wave wave_muse[] = {
  {6, K_DRONE, M_SWOOP_L, 0, 8, 8, 0}, {7, K_DRONE, M_SWOOP_R, 0, 8, 8, 0},
  {11, K_GUNNER, M_HOVER, 16, 4, 12, 40}, {16, K_HEAVY, M_DIVE, 64, 1, 0, 0},
  {19, K_DRONE, M_SNAKE, 32, 10, 7, 0}, {20, K_DRONE, M_SNAKE, 112, 10, 7, 0},
  {25, K_GUNNER, M_SIDE_L, 16, 4, 24, 0}, {26, K_GUNNER, M_SIDE_R, 40, 4, 24, 0},
  {31, K_HEAVY, M_HOVER, 24, 2, 0, 80}, {35, K_DRONE, M_DIVE, 16, 10, 7, 14},
  {38, K_DRONE, M_DIVE, 136, 10, 7, -14}, {42, K_GUNNER, M_HOVER, 72, 1, 0, 0},
  {43, K_GUNNER, M_DIVE, 16, 4, 20, 40}, {48, K_DRONE, M_SWOOP_L, 0, 10, 7, 0},
  {49, K_DRONE, M_SWOOP_R, 0, 10, 7, 0}, {55, K_HEAVY, M_DIVE, 16, 3, 40, 48},
  {61, K_GUNNER, M_SIDE_L, 12, 4, 20, 0}, {62, K_GUNNER, M_SIDE_R, 36, 4, 20, 0},
  {68, K_DRONE, M_SNAKE, 72, 12, 6, 0}, {72, K_GUNNER, M_HOVER, 16, 4, 12, 40},
  {78, K_HEAVY, M_HOVER, 64, 1, 0, 0}, {80, K_DRONE, M_SWOOP_L, 0, 8, 8, 0},
  {82, K_DRONE, M_SWOOP_R, 0, 8, 8, 0}, {88, K_GUNNER, M_DIVE, 24, 5, 16, 24},
  {94, K_DRONE, M_DIVE, 16, 12, 6, 12}, {99, K_HEAVY, M_HOVER, 24, 2, 0, 80},
  {104, K_GUNNER, M_SIDE_L, 20, 4, 20, 0}, {105, K_GUNNER, M_SIDE_R, 44, 4, 20, 0},
  {111, K_DRONE, M_SNAKE, 40, 10, 7, 0}, {112, K_DRONE, M_SNAKE, 104, 10, 7, 0},
  {118, K_HEAVY, M_DIVE, 16, 4, 40, 36}, {125, K_GUNNER, M_HOVER, 16, 4, 12, 40},
  {131, K_DRONE, M_SWOOP_L, 0, 10, 7, 0}, {132, K_DRONE, M_SWOOP_R, 0, 10, 7, 0},
  {138, K_HEAVY, M_HOVER, 64, 1, 0, 0}, END};
static const Wave * const scripts[4] = {wave_grok, wave_claude, wave_gemini, wave_muse};


/* Boss tuning: hit points and the core's offset inside the 96x56 art. */
static const uint16_t boss_hp[4] = {320, 400, 480, 560};
static const uint8_t core_x[4] = {40, 40, 40, 40};
static const uint8_t core_y[4] = {20, 20, 20, 30};


typedef struct { uint8_t kind, mode, x, count, gap, timer; int8_t dx; } Group;
static Group groups[4];
static const Wave *script;

static uint8_t aim(int16_t dx, int16_t dy) {
  uint8_t ax = gap(dx, 0), ay = gap(dy, 0), a;
  if (!ax && !ay) return 8;
  if (ax >= ay) a = atan_tab[((uint16_t)ay << 4) / ax];
  else a = 8u - atan_tab[((uint16_t)ax << 4) / ay];
  if (dx >= 0) return dy >= 0 ? a : (uint8_t)(32u - a) & 31u;
  return dy >= 0 ? 16u - a : 16u + a;
}


static uint8_t free_hint;

static void bullet(int16_t x, int16_t y, uint8_t dir, uint8_t speed, uint8_t kind) {
  uint8_t i, n, cls;
  DwShot *b;
  if (y > 124 || x < -4 || x > 164) return;
  i = free_hint;
  for (n = DW_BULLETS; n; --n) {
    b = &dw_bullets[i];
    if (++i == DW_BULLETS) i = 0;
    if (b->on) continue;
    free_hint = i;
    dir &= 31u;
    cls = speed_class[speed];
    b->x = POS(x);
    b->y = POS(y);
    b->vx = vel_x[cls][dir];
    b->vy = vel_y[cls][dir];
    b->kind = DW_S_BULLET + (kind << 1);
    b->on = 1;
    return;
  }
}

/* Evenly spaced ring of n bullets starting at direction `first`. */
static void ring(int16_t x, int16_t y, uint8_t n, uint8_t first, uint8_t speed, uint8_t kind) {
  uint16_t step = 8192u / n, acc = (uint16_t)first << 8;
  while (n--) {
    bullet(x, y, (uint8_t)(acc >> 8), speed, kind);
    acc += step;
  }
}

static uint8_t to_player(int16_t x, int16_t y) {
  return aim(dw_px + 8 - x, dw_py + 7 - y);
}


static void spawn(Group *g) {
  uint8_t i, s = SECTOR_IDX;
  DwFoe *e = dw_foes;
  for (i = 0; i < DW_FOES; ++i, ++e) if (!e->hp) break;
  if (i == DW_FOES) return;
  e->kind = g->kind;
  e->mode = g->mode;
  e->t = 0;
  e->flash = 0;
  e->cool = g->kind == K_GUNNER ? 30 : g->kind == K_HEAVY ? 40 : 36;
  e->pad = 0;
  e->arg = g->x;
  e->vx = 0;
  e->hp = g->kind == K_DRONE ? 1u + (s >> 1) : g->kind == K_GUNNER ? 5u + s : 22u + s * 6u;
  e->x = POS(g->x);
  e->y = POS(-16);
  switch (g->mode) {
    case M_DIVE: e->vy = g->kind == K_HEAVY ? 112 : g->kind == K_GUNNER ? 224 : 288 + s * 32; break;
    case M_SWOOP_L: e->x = POS(-14); e->y = POS(4); e->vx = 416; e->vy = 160; break;
    case M_SWOOP_R: e->x = POS(158); e->y = POS(4); e->vx = -416; e->vy = 160; break;
    case M_SNAKE: e->vy = 256 + s * 16; break;
    case M_HOVER: e->vy = 352; break;
    case M_SIDE_L: e->x = POS(-14); e->y = POS(g->x); e->vx = 288 + s * 32; e->vy = 0; break;
    case M_SIDE_R: e->x = POS(158); e->y = POS(g->x); e->vx = -288 - (int16_t)(s * 32); e->vy = 0; break;
  }
  if (g->kind == K_HEAVY && g->mode != M_SIDE_L && g->mode != M_SIDE_R) e->x = POS(g->x - 8);
}

void dw_waves(uint8_t step) BANKED {
  uint8_t i;
  Group *g;
  while (script[wave_i].at != 255 && script[wave_i].at <= step) {
    const Wave *w = &script[wave_i++];
    for (i = 0, g = groups; i < 4; ++i, ++g) if (!g->count) {
      g->kind = w->kind;
      g->mode = w->mode;
      g->x = w->x;
      g->count = w->count;
      g->gap = w->gap;
      g->dx = w->dx;
      g->timer = 0;
      break;
    }
  }
  for (i = 0, g = groups; i < 4; ++i, ++g) {
    if (!g->count) continue;
    if (g->timer) {
      --g->timer;
      continue;
    }
    spawn(g);
    --g->count;
    g->timer = g->gap;
    g->x = (uint8_t)(g->x + g->dx);
  }
}



/* Rivals are processed through a static working copy: SDCC reaches its
   fields with direct addresses, far cheaper than through a pointer. */
static DwFoe cur;

/* Copy one 16-byte rival record: destination in DE, source in BC. */
static void copy16(void *dst, const void *src) __naked {
  (void)dst; (void)src;
  __asm
    ld h, b
    ld l, c
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ld a, (hl+)
    ld (de), a
    inc de
    ret
  __endasm;
}
#define BASE_X ((uint16_t)(uint8_t)(cur.arg + BIAS) << 8)

static void cur_fire(void) {
  uint8_t s = SECTOR_IDX, d, n;
  int16_t x = (int16_t)HI(cur.x) - BIAS + (cur.kind == K_HEAVY ? 16 : 8);
  int16_t y = (int16_t)HI(cur.y) - BIAS + 10;
  if (cur.kind == K_HEAVY) {
    /* Half the ring now, the other half next frame: a quick double pulse
       that also halves the frame's spawning work. */
    n = 4u + s;
    ring(x, y, n, cur.t >> 3, 16 + s * 2, s == 2 ? B_STAR : B_BIG);
    cur.pad = 0x80u | (uint8_t)((cur.t >> 3) + 16u / n);
    return;
  }
  d = to_player(x, y);
  if (cur.kind == K_DRONE) {
    bullet(x, y, d, 18 + s * 3, B_SMALL);
  } else {
    bullet(x, y, d, 22 + s * 3, s & 1 ? B_NEEDLE : B_SMALL);
    if (s) {
      bullet(x, y, d - 2, 22 + s * 3, B_SMALL);
      bullet(x, y, d + 2, 22 + s * 3, B_SMALL);
    }
  }
}

static void cur_move(uint8_t armed) {
  uint8_t t = ++cur.t;
  int16_t w;
  switch (cur.mode) {
    case M_DIVE:
      cur.y += cur.vy;
      if (cur.kind == K_DRONE) cur.x = BASE_X + wave16[(t << 1) & 63];
      break;
    case M_SWOOP_L:
    case M_SWOOP_R:
      cur.x += cur.vx;
      cur.y += cur.vy;
      if (t > 22 && !(t & 1)) {
        if (cur.mode == M_SWOOP_L) { if (cur.vx > -288) cur.vx -= 16; }
        else if (cur.vx < 288) cur.vx += 16;
        if (cur.vy < 416) cur.vy += 16;
      }
      break;
    case M_SNAKE:
      cur.y += cur.vy;
      w = wave16[(uint8_t)(t + (t << 1)) & 63];
      cur.x = BASE_X + w + (w >> 1);
      break;
    case M_HOVER:
      if (t < 200) {
        if (HI(cur.y) < BIAS + 30) cur.y += cur.vy;
        w = wave16[(t >> 1) & 63];
        cur.x = BASE_X + (w >> 1) + (w >> 2);
      } else {
        cur.y += 320;
      }
      break;
    default:
      cur.x += cur.vx;
      cur.y = BASE_X + (wave16[(t << 1) & 63] >> 1);
      break;
  }
  if (!armed) return;
  if (cur.pad) {
    uint8_t s = SECTOR_IDX;
    ring((int16_t)HI(cur.x) - BIAS + 16, (int16_t)HI(cur.y) - BIAS + 10, 4u + s, cur.pad & 31u,
         16 + s * 2, s == 2 ? B_STAR : B_BIG);
    cur.pad = 0;
  }
  if (--cur.cool) return;
  if (cur.kind == K_DRONE) {
    cur.cool = 255;
    if (dw_sector > 1 && !(dw_random() & 3u)) cur_fire();
  } else if (cur.kind == K_GUNNER) {
    cur.cool = 72u - dw_sector * 8u;
    cur_fire();
  } else {
    cur.cool = 56;
    cur_fire();
  }
}


static void boss_shoot(void) {
  uint8_t s = SECTOR_IDX, i, d, k;
  int16_t cx = dw_boss_x + 48, cy = dw_boss_y + 30;
  if (boss_fire) {
    --boss_fire;
    return;
  }
  ++boss_step;
  d = to_player(cx, cy);
  switch (s * 3u + boss_phase) {
    /* Grok: aimed fans, then rotating rings, then a spiral storm. */
    case 0:
      for (i = 0; i < 3; ++i) bullet(cx, cy, d - 2 + i * 2, 22, B_SMALL);
      bullet(dw_boss_x + 10, dw_boss_y + 34, 8, 20, B_BIG);
      bullet(dw_boss_x + 86, dw_boss_y + 34, 8, 20, B_BIG);
      boss_fire = 46;
      break;
    case 1:
      ring(cx, cy, 12, (boss_step & 1u) + boss_spin, 18, B_BIG);
      boss_spin += 1;
      if (boss_step & 1u) bullet(cx, cy, d, 34, B_NEEDLE);
      boss_fire = 38;
      break;
    case 2:
      boss_spin += 3;
      bullet(cx, cy, boss_spin, 20, B_SMALL);
      bullet(cx, cy, boss_spin + 16, 20, B_SMALL);
      if (!(boss_step % 10u)) for (i = 0; i < 5; ++i) bullet(cx, cy, d - 4 + i * 2, 26, B_BIG);
      boss_fire = 5;
      break;
    /* Claude: sunburst rings and twin spirals. */
    case 3:
      ring(cx, cy, 10, (boss_step & 1u) * 2u, 18, B_BIG);
      boss_fire = 42;
      break;
    case 4:
      ring(cx, cy, 8, boss_spin, 20, B_SMALL);
      boss_spin += 2;
      if (!(boss_step % 3u)) for (i = 0; i < 3; ++i) bullet(cx, cy, d - 2 + i * 2, 28, B_BIG);
      boss_fire = 24;
      break;
    case 5:
      boss_spin += 2;
      ring(cx, cy, 4, boss_spin, 19, B_SMALL);
      if (!(boss_step % 8u)) for (i = 0; i < 5; ++i) bullet(cx, cy, d - 4 + i * 2, 28, B_BIG);
      boss_fire = 7;
      break;
    /* Gemini: twin stars fire in alternation, then crossing streams. */
    case 6:
      k = boss_step & 1u ? 24 : 72;
      d = to_player(dw_boss_x + k, cy);
      for (i = 0; i < 3; ++i) bullet(dw_boss_x + k, cy, d - 2 + i * 2, 24, B_STAR);
      boss_fire = 24;
      break;
    case 7:
      boss_spin += 1;
      d = 8u + (sin64[(boss_spin << 2) & 63] >> 4);
      bullet(dw_boss_x + 24, cy, d - 3, 26, B_STAR);
      bullet(dw_boss_x + 72, cy, 19u - d, 26, B_STAR);
      if (!(boss_step % 12u)) bullet(cx, cy, to_player(cx, cy), 32, B_NEEDLE);
      boss_fire = 5;
      break;
    case 8:
      ring(dw_boss_x + 24, cy, 8, boss_spin, 20, B_STAR);
      ring(dw_boss_x + 72, cy, 8, 2u - boss_spin, 20, B_STAR);
      boss_spin += 1;
      boss_fire = 26;
      break;
    /* Muse: V walls, aimed volleys from the wings, then a dense spiral. */
    default:
      if (boss_phase == 0) {
        for (i = 0; i < 4; ++i) {
          bullet(cx - 4 - i * 6, cy - 4 * i, 8 - 1, 18, B_NEEDLE);
          bullet(cx + 4 + i * 6, cy - 4 * i, 8 + 1, 18, B_NEEDLE);
        }
        bullet(cx, cy, d, 24, B_BIG);
        boss_fire = 40;
      } else if (boss_phase == 1) {
        for (i = 0; i < 5; ++i) bullet(cx, cy, d - 4 + i * 2, 26, B_SMALL);
        bullet(dw_boss_x + 12, dw_boss_y + 44, to_player(dw_boss_x + 12, dw_boss_y + 44), 30, B_NEEDLE);
        bullet(dw_boss_x + 84, dw_boss_y + 44, to_player(dw_boss_x + 84, dw_boss_y + 44), 30, B_NEEDLE);
        boss_fire = 26;
      } else {
        boss_spin += 5;
        bullet(cx, cy, boss_spin, 22, B_NEEDLE);
        bullet(cx, cy, boss_spin + 11, 22, B_NEEDLE);
        bullet(cx, cy, boss_spin + 22, 22, B_NEEDLE);
        if (!(boss_step % 9u)) bullet((int16_t)(dw_random() & 127u) + 16, dw_boss_y + 50, 8, 26, B_BIG);
        boss_fire = 6;
      }
      break;
  }
}


void dw_rivals_begin(void) BANKED {
  uint8_t i;
  wave_i = 0;
  script = scripts[SECTOR_IDX];
  for (i = 0; i < 4; ++i) groups[i].count = 0;
}

void dw_boss_begin(void) BANKED {
  dw_boss_max = dw_boss_hp = boss_hp[SECTOR_IDX];
  boss_third = dw_boss_max / 3u;
  boss_two_thirds = boss_third << 1;
  dw_boss_t = boss_phase = boss_step = boss_spin = 0;
  boss_fire = 90;
}

/* Sway, pick the attack phase from remaining health, and fire. */
void dw_boss_tick(void) BANKED {
  ++dw_boss_t;
  {
    int8_t s = sin64[(dw_boss_t >> 1) & 63];
    dw_boss_x = DW_BOSS_LEFT + (s >> 2) + (s >> 3);
  }
  dw_boss_y = 8 + (sin64[dw_boss_t & 63] >> 4);
  if (dw_boss_hp < boss_third) boss_phase = 2;
  else if (dw_boss_hp < boss_two_thirds) boss_phase = 1;
  boss_shoot();
}

/* Move every rival, let it fire, and retire any that leave the sky. */
void dw_foes_tick(uint8_t armed) BANKED {
  uint8_t i;
  DwFoe *e = dw_foes;
  for (i = 0; i < DW_FOES; ++i, ++e) {
    if (!e->hp) continue;
    copy16(&cur, e);
    cur_move(armed);
    if (HI(cur.y) > BIAS + 140 || HI(cur.x) < 2 || HI(cur.x) > BIAS + 196) cur.hp = 0;
    copy16(e, &cur);
  }
}
