/* Gameplay: the Data Analyst floats above the lunar Dreambase and eats the
   partner logos its turrets fire upward. Fill the meter to absorb each
   company and transform; eight companies end in the Dreambase final form.

   Tuning follows the web edition's level table (team/GAMEPLAY.md): fire
   intervals, cannon counts, pattern mixes, catches per level, points and
   life drain. Distances are scaled to the 160 x 96 pixel sky. */
#include "dreambase.h"
static const uint8_t sine32[32] = SINE32_INIT;

/* Level table, indexed by level 0-7. */
static const uint8_t L_CANNONS[8] = {3, 3, 4, 4, 4, 5, 5, 5};
static const uint8_t L_INTERVAL[8] = {78, 69, 60, 54, 48, 43, 38, 33};
static const uint8_t L_SPEED[8] = {26, 30, 34, 38, 44, 50, 57, 65};
static const uint8_t L_CATCH[8] = {10, 12, 14, 16, 18, 20, 22, 25};
static const uint8_t L_POINTS[8] = {10, 15, 20, 25, 30, 40, 50, 60};
static const uint8_t L_DRAIN[8] = {8, 9, 10, 11, 12, 12, 13, 14};
/* Cumulative chances out of 100: straight, aimed, spread; the rest wave. */
static const uint8_t L_MIX[8][3] = {{100, 100, 100}, {80, 100, 100}, {70, 70, 100},
                                    {50, 70, 70},    {40, 80, 100},  {0, 20, 60},
                                    {0, 50, 70},     {25, 50, 75}};
static const uint8_t SLOT_X[5] = {24, 48, 80, 112, 136};
static const uint8_t MUZZLE_Y[5] = {112, 112, 108, 112, 112};
static const uint8_t SLOT_MASK[6] = {0, 0, 0, 0x0E, 0x1B, 0x1F};
static const char *const names[8] = {"SUPABASE", "STRIPE", "POSTHOG", "VERCEL",
                                     "GITHUB", "CLICKHOUSE", "LINEAR", "DREAMBASE"};
static const uint8_t heartbeat[4] = {45, 43, 41, 40};
static const int8_t dir_x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int8_t dir_y[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

#define LIFE_MAX 96
#define P_PLAY 0
#define P_LEVEL 1
#define P_DYING 2
#define P_INTRO 3
#define PAT_STRAIGHT 1
#define PAT_WAVE 2

/* Exported for the emulator tests. */
int16_t pl_x, pl_y;
uint8_t pl_phase, pl_paused, pl_catches, pl_need, pl_life, pl_lives, pl_streak, pl_form;
uint16_t pl_timer;

static uint8_t interval, speed, mask, next_shot, hold, order_pos, perfect, extra;
static uint8_t tier, chomp_t, flash_t, inv_t, dash_t, dash_cd, shake_t, miss_t, white_t;
static uint8_t face, beat_t, beat_n, star_t, shown_life, shown_meter, splash_row;
static uint8_t tel[5], lastpat[5], order[5];
uint8_t pp_life[2];
static uint8_t pp_n[2], pp_ch[2][4];
static int16_t pp_x[2], pp_y[2];
static int8_t ox, oy;
static uint16_t shown_score, bonus, meter_fx, meter_step;

/* ------------------------------------------------------------- helpers */
static void popup(int16_t x, int16_t y, const char *s);
static void splash_text(uint8_t announce);
static void add_score(uint16_t n) {
  db_score = db_score > 65535u - n ? 65535u : db_score + n;
  if (!extra && db_score >= 5000) {
    extra = 1;
    if (pl_lives < 9)
      pl_lives++;
    music(SONG_ONEUP);
    popup(pl_x >> FX, (pl_y >> FX) - 10, "1UP!");
  }
}
static void popup(int16_t x, int16_t y, const char *s) {
  uint8_t k = pp_life[0] > pp_life[1] ? 1 : 0, n = 0, c;
  while ((c = *s++) && n < 4)
    pp_ch[k][n++] = c >= '0' && c <= '9' ? c - '0' : c == 'X' ? 10 : c == '!' ? 11 : c == 'U' ? 12 : c == 'P' ? 13 : 14;
  pp_n[k] = n;
  pp_x[k] = x - n * 2;
  pp_y[k] = y - 10;
  pp_life[k] = 40;
}
static void popup_points(int16_t x, int16_t y, uint16_t v) {
  char s[5];
  uint8_t n = 1, h = 0, t = 0;
  s[0] = '+';
  while (v >= 100) {
    v -= 100;
    h++;
  }
  while (v >= 10) {
    v -= 10;
    t++;
  }
  if (h)
    s[n++] = '0' + h;
  if (h || t)
    s[n++] = '0' + t;
  s[n++] = '0' + (uint8_t)v;
  s[n] = 0;
  popup(x, y, s);
}
static void spark(int16_t x, int16_t y, int16_t vx, int16_t vy, uint8_t life, uint8_t tile, uint8_t pal) {
  uint8_t i;
  for (i = 0; i < MAXK; i++)
    if (!pk_life[i])
      break;
  if (i == MAXK)
    i = db_rand() & (MAXK - 1);
  pk_life[i] = life;
  pk_x[i] = x << FX;
  pk_y[i] = y << FX;
  pk_vx[i] = vx;
  pk_vy[i] = vy;
  pk_tile[i] = tile;
  pk_pal[i] = pal;
}
/* Eight directions at a brisk speed; power 0 halves it, 2 adds half. */
static const int8_t burst_vx[8] = {64, 45, 0, -45, -64, -45, 0, 45};
static const int8_t burst_vy[8] = {0, -45, -64, -45, 0, 45, 64, 45};
static void burst(int16_t x, int16_t y, uint8_t n, uint8_t pal, uint8_t power) {
  uint8_t k = db_rand(), r = k;
  int16_t vx, vy;
  while (n--) {
    k += 3;
    r = (r << 1) | (r >> 7);
    vx = burst_vx[k & 7];
    vy = burst_vy[k & 7];
    if (power == 0) {
      vx >>= 1;
      vy >>= 1;
    } else if (power == 2) {
      vx += vx >> 1;
      vy += vy >> 1;
    }
    spark(x, y, vx, vy, 14 + (r & 7), SPR_SPARK_N(r & 3), pal);
  }
}
static void tint_sky(uint16_t c) {
  PAL_LIVE[16] = c;
  db_pal_dirty |= 1 << 4;
}
static void hud_pal(uint8_t slot, uint16_t c1, uint16_t c2, uint16_t c3) {
  uint16_t c[4];
  c[0] = RGB(2, 3, 5);
  c[1] = c1;
  c[2] = c2;
  c[3] = c3;
  db_pal(slot, c);
}
static void life_pal(void) {
  if ((pl_life >> 1) < 15)
    hud_pal(2, RGB(5, 6, 9), RGB(18, 3, 6), RGB(31, 7, 11));
  else
    hud_pal(2, RGB(5, 6, 9), RGB(4, 18, 11), RGB(13, 30, 21));
}

/* ----------------------------------------------------------------- HUD */
static void bar(uint8_t x, uint8_t y, uint8_t tiles, uint8_t px, uint8_t pal) {
  uint8_t i;
  for (i = 0; i < tiles; i++, px = px > 8 ? px - 8 : 0)
    db_put(x + i, y, T_BAR0 + (px > 8 ? 8 : px), pal);
}
static void hud_name(uint8_t level) {
  const char *s = names[level];
  db_text(6, 4, "          ", 1);
  db_text(6 + ((10 - db_strlen(s)) >> 1), 4, s, 1);
}
static void hud_lives(void) {
  db_put(17, 4, T_LIFE, 0);
  db_text(18, 4, "X", 0);
  db_put(19, 4, 17 + pl_lives, 0);
}
static void hud_all(void) {
  db_digits(0, 4, db_score, 5, 0);
  shown_score = db_score;
  hud_name(db_level);
  hud_lives();
  db_put(0, 5, T_HEART, 2);
  db_put(11, 5, T_MINI0 + db_level, 1);
  shown_life = shown_meter = 255;
}
static void hud(void) {
  uint8_t v;
  if (shown_score != db_score) {
    db_digits(0, 4, db_score, 5, 0);
    shown_score = db_score;
  }
  v = pl_life >> 1;
  if (v != shown_life) {
    if ((shown_life < 15) != (v < 15))
      life_pal();
    shown_life = v;
    bar(1, 5, 6, v, 2);
  }
  v = (uint8_t)(meter_fx >> 8);
  if (v != shown_meter) {
    shown_meter = v;
    bar(12, 5, 8, v, 1);
  }
}
static void hud_combo(void) {
  static const char *const tags[4] = {"  ", "X2", "X4", "X8"};
  db_text(8, 5, tags[tier], 0);
}

/* -------------------------------------------------------------- levels */
static void company_colours(uint8_t level) {
  uint16_t c[4];
  const uint16_t *k = company_cols + level * 3;
  hud_pal(1, RGB(5, 6, 9), k[0], k[2]);
  c[0] = pal_flash[0];
  c[1] = pal_flash[1];
  c[2] = pal_flash[2];
  c[3] = k[0];
  db_pal(7, c);
  db_pal(OBJ + 1, logo_pals + level * 4);
  c[0] = 0;
  c[1] = k[1];
  c[2] = k[0];
  c[3] = k[2];
  db_pal(OBJ + 2, c);
}
static void shuffle(void) {
  uint8_t i, j, t, n = 0;
  for (i = 0; i < 5; i++)
    if (mask & (1 << i))
      order[n++] = i;
  for (i = n - 1; i; i--) {
    j = db_rand() % (i + 1);
    t = order[i];
    order[i] = order[j];
    order[j] = t;
  }
  order_pos = 0;
}
static void level_setup(uint8_t level) {
  uint8_t i, base;
  db_level = level;
  mask = SLOT_MASK[L_CANNONS[level]];
  base = L_CATCH[level];
  pl_need = base + ((base * db_diff) >> 1);
  interval = L_INTERVAL[level] - (uint8_t)((L_INTERVAL[level] * db_diff) / 10);
  speed = L_SPEED[level] + (uint8_t)((L_SPEED[level] * db_diff) / 10);
  pl_catches = 0;
  meter_fx = 0;
  meter_step = (uint16_t)((64u << 8) / pl_need);
  perfect = 1;
  for (i = 0; i < 5; i++) {
    tel[i] = lastpat[i] = 0;
    art_cannon(i, (mask >> i) & 1, 7);
  }
  for (i = 0; i < MAXP; i++)
    pr_on[i] = 0;
  company_colours(level);
  hud_all();
  hud_combo();
  shuffle();
  next_shot = 1;
}
static void form_pal(uint8_t f) {
  db_pal(OBJ, form_pals + f * 4);
}

void play_enter(void) DB_BANKED {
  uint8_t i;
  db_screen_off();
  db_mode = M_PLAY;
  db_score = 0;
  db_won = 0;
  pl_lives = 3;
  pl_life = LIFE_MAX;
  pl_streak = tier = extra = 0;
  pl_form = 0;
  pl_phase = P_PLAY;
  pl_paused = 0;
  pl_x = 80 << FX;
  pl_y = 70 << FX;
  chomp_t = flash_t = inv_t = dash_t = dash_cd = shake_t = miss_t = white_t = 0;
  face = 0;
  ox = oy = 0;
  for (i = 0; i < MAXK; i++)
    pk_life[i] = 0;
  pp_life[0] = pp_life[1] = 0;
  art_stars(0, 31);
  db_map_hi = 0x9C;
  db_clear_rows(0, MAP_ROWS - 1);
  art_base(0, 3);
  db_scx = db_scy = 0;
  db_wx = 7;
  db_wy = 96;
  SHOW_WIN;
  hud_pal(0, RGB(5, 6, 9), RGB(19, 21, 26), RGB(31, 31, 31));
  life_pal();
  db_pal(3, pal_flash);
  db_pal(4, pal_stars);
  db_pal(5, pal_moon);
  db_pal(6, pal_base);
  db_pal(OBJ + 3, pal_gold);
  db_pal_fill(OBJ + 4, RGB(31, 31, 31));
  PAL_TARGET[(OBJ + 4) * 4 + 1] = PAL_TARGET[(OBJ + 4) * 4 + 2] = PAL_TARGET[(OBJ + 4) * 4 + 3] = RGB(31, 31, 31);
  form_pal(0);
  level_setup(0);
  db_pal(OBJ + 5, logo_pals);
  hold = 110;
  beat_t = 30;
  bonus = 0;
  pl_phase = P_INTRO;
  pl_timer = 0;
  splash_text(0);
  db_screen_on();
  db_fade_to(8);
}

/* ------------------------------------------------------------- firing */
static void spawn(uint8_t slot, int16_t vx, uint8_t pattern) {
  uint8_t i;
  for (i = 0; i < MAXP; i++)
    if (!pr_on[i])
      break;
  if (i == MAXP)
    return;
  pr_on[i] = pattern;
  pr_x[i] = (int16_t)SLOT_X[slot] << FX;
  pr_y[i] = (int16_t)MUZZLE_Y[slot] << FX;
  pr_vx[i] = vx;
  pr_vy[i] = -(int16_t)speed;
  pr_bx[i] = SLOT_X[slot];
  pr_ph[i] = db_rand();
  pr_logo[i] = db_level;
}
static void fire(uint8_t slot) {
  uint8_t r = db_rand() % 100, pattern;
  const uint8_t *mix = L_MIX[db_level];
  int16_t dx, dy, vx, lim;
  pattern = r < mix[0] ? 0 : r < mix[1] ? 1 : r < mix[2] ? 2 : 3;
  if (pattern == 1 && lastpat[slot] == 1)
    pattern = 0;
  lastpat[slot] = pattern;
  if (pattern == 0)
    spawn(slot, 0, PAT_STRAIGHT);
  else if (pattern == 1) {
    /* Aimed at the analyst, never more than about 35 degrees off vertical. */
    dx = (pl_x >> FX) - SLOT_X[slot];
    dy = MUZZLE_Y[slot] - (pl_y >> FX);
    if (dy < 16)
      dy = 16;
    vx = ((int16_t)speed * dx) / dy;
    lim = ((int16_t)speed * 7) / 10;
    spawn(slot, vx > lim ? lim : vx < -lim ? -lim : vx, PAT_STRAIGHT);
  } else if (pattern == 2) {
    spawn(slot, -(int16_t)(speed >> 2), PAT_STRAIGHT);
    spawn(slot, 0, PAT_STRAIGHT);
    spawn(slot, speed >> 2, PAT_STRAIGHT);
  } else
    spawn(slot, 0, PAT_WAVE);
  spark(SLOT_X[slot], MUZZLE_Y[slot] - 10, 0, -40, 8, SPR_SPARK_N(6), 2);
  sfx(SFX_FIRE, 0);
}
static void telegraph(uint8_t slot, uint8_t on) {
  if (slot == 2) {
    PAL_LIVE[6 * 4 + 3] = on ? RGB(31, 31, 31) : pal_base[3];
    db_pal_dirty |= 1 << 6;
    return;
  }
  art_cannon(slot, 1, on ? 3 : 7);
}
static void update_fire(void) {
  uint8_t i, jitter;
  for (i = 0; i < 5; i++) {
    if (tel[i] && !--tel[i]) {
      telegraph(i, 0);
      fire(i);
    }
  }
  if (hold) {
    hold--;
    return;
  }
  if (--next_shot)
    return;
  if (order_pos >= 5 || !(mask & (1 << order[order_pos])) || order_pos >= L_CANNONS[db_level])
    shuffle();
  i = order[order_pos++];
  tel[i] = 15;
  telegraph(i, 1);
  jitter = interval / 5;
  next_shot = interval - jitter / 2 + db_rand() % (jitter + 1);
}

/* -------------------------------------------------------- catch and miss */
static void lose_life(void) {
  uint8_t i;
  for (i = 0; i < MAXP; i++)
    pr_on[i] = 0;
  for (i = 0; i < 5; i++)
    if (tel[i]) {
      tel[i] = 0;
      telegraph(i, 0);
    }
  pl_streak = 0;
  tier = 0;
  hud_combo();
  perfect = 0;
  shake_t = 20;
  white_t = 3;
  for (i = 0; i < 8; i++)
    db_pal_fill(i, RGB(31, 31, 31));
  burst(pl_x >> FX, pl_y >> FX, 12, 2, 2);
  sfx(SFX_LOST, 0);
  pl_lives--;
  hud_lives();
  if (!pl_lives) {
    pl_life = 0;
    pl_phase = P_DYING;
    pl_timer = 0;
    return;
  }
  pl_life = LIFE_MAX;
  inv_t = 90;
  hold = 90;
  pl_x = 80 << FX;
  pl_y = 44 << FX;
}
static void miss(void) {
  pl_streak = 0;
  if (tier) {
    tier = 0;
    hud_combo();
  }
  perfect = 0;
  miss_t = 8;
  shake_t = shake_t > 6 ? shake_t : 6;
  sfx(SFX_MISS, 0);
  if (inv_t)
    return;
  if (pl_life <= L_DRAIN[db_level])
    lose_life();
  else
    pl_life -= L_DRAIN[db_level];
}
static void begin_level_up(void);
static void catch_logo(uint8_t i) {
  uint8_t t;
  uint16_t pts;
  int16_t x = pr_x[i] >> FX, y = pr_y[i] >> FX;
  pr_on[i] = 0;
  if (pl_streak < 255)
    pl_streak++;
  t = pl_streak >= 15 ? 3 : pl_streak >= 10 ? 2 : pl_streak >= 5 ? 1 : 0;
  pts = (uint16_t)L_POINTS[db_level] << t;
  add_score(pts);
  if (t != tier) {
    tier = t;
    hud_combo();
    popup(x, y - 8, t == 1 ? "X2!" : t == 2 ? "X4!" : "X8!!");
    sfx(SFX_TIER, (t - 1) * 3);
  } else {
    popup_points(x, y, pts);
    sfx(SFX_EAT, t << 1);
  }
  pl_life = pl_life + 2 > LIFE_MAX ? LIFE_MAX : pl_life + 2;
  chomp_t = 8;
  flash_t = 3;
  burst(x, y, 4, 2, 0);
  meter_fx += meter_step;
  if (++pl_catches >= pl_need) {
    meter_fx = 64u << 8;
    begin_level_up();
  }
}
static void update_projectiles(void) {
  uint8_t i;
  int16_t x, y, px = pl_x >> FX, py = pl_y >> FX;
  for (i = 0; i < MAXP; i++) {
    if (!pr_on[i])
      continue;
    pr_y[i] += pr_vy[i];
    if (pr_on[i] == PAT_WAVE) {
      pr_ph[i]++;
      x = (int8_t)sine32[(pr_ph[i] >> 1) & 31];
      x = pr_bx[i] + ((x + x + x) >> 4);
      pr_x[i] = x << FX;
    } else {
      pr_x[i] += pr_vx[i];
      if (pr_x[i] < (8 << FX) || pr_x[i] > (152 << FX)) {
        pr_vx[i] = -pr_vx[i];
        pr_x[i] += pr_vx[i];
      }
      x = pr_x[i] >> FX;
    }
    y = pr_y[i] >> FX;
    if (pl_phase == P_PLAY && x - px < 12 && px - x < 12 && y - py < 12 && py - y < 12) {
      catch_logo(i);
      if (pl_phase != P_PLAY)
        return;
    } else if (y < -8) {
      pr_on[i] = 0;
      miss();
      if (pl_phase != P_PLAY)
        return;
    }
  }
}

/* --------------------------------------------------------------- player */
static void update_player(void) {
  uint8_t k = db_keys, d = 255;
  int16_t step;
  if (k & J_UP)
    d = k & J_RIGHT ? 1 : k & J_LEFT ? 7 : 0;
  else if (k & J_DOWN)
    d = k & J_RIGHT ? 3 : k & J_LEFT ? 5 : 4;
  else if (k & J_RIGHT)
    d = 2;
  else if (k & J_LEFT)
    d = 6;
  if (d != 255)
    face = d;
  if (dash_cd)
    dash_cd--;
  if ((db_pressed & (J_A | J_B)) && !dash_cd) {
    dash_t = 8;
    dash_cd = 40;
    sfx(SFX_DASH, 0);
  }
  if (dash_t) {
    dash_t--;
    d = face;
    step = d & 1 ? 180 : 256;
    if (dash_t & 1)
      spark(pl_x >> FX, pl_y >> FX, 0, 0, 10, SPR_SPARK_N(4), 2);
  } else
    step = d & 1 ? 68 : 96;
  if (d != 255) {
    pl_x += dir_x[d] * step;
    pl_y += dir_y[d] * step;
  }
  if (pl_x < (10 << FX))
    pl_x = 10 << FX;
  if (pl_x > (150 << FX))
    pl_x = 150 << FX;
  if (pl_y < (10 << FX))
    pl_y = 10 << FX;
  if (pl_y > (76 << FX))
    pl_y = 76 << FX;
}

/* ------------------------------------------------------- level-up show */
/* The level banner: six rows of the scrolling sky, written relative to the
   frozen scroll so it sits still on screen. */
static void banner_row(uint8_t r, const uint8_t *tiles, const uint8_t *att) {
  VBK_REG = 1;
  set_bkg_tiles(0, r & 31, 32, 1, att);
  VBK_REG = 0;
  set_bkg_tiles(0, r & 31, 32, 1, tiles);
}
static void put_text(uint8_t *tiles, uint8_t x, const char *s) {
  for (; *s; s++, x++)
    tiles[x] = *s == ' ' ? 0 : *s - 31;
}
/* announce: the level index (0-7) to introduce, or 8 for the final form. */
static void splash_text(uint8_t announce) {
  uint8_t r = (uint8_t)((db_scy >> 3) + 3) & 31, i, t, len, x, tiles[32], att[32];
  const char *name = names[announce > 7 ? 7 : announce];
  splash_row = r;
  for (t = 0; t < 6; t++) {
    for (i = 0; i < 32; i++) {
      tiles[i] = 0;
      att[i] = 0;
    }
    if (t == 0) {
      if (announce > 7)
        put_text(tiles, 2, "ALL DATA EATEN!");
      else {
        put_text(tiles, 6, "LEVEL");
        tiles[13] = 17 + announce + 1;
      }
    } else if ((t == 2 || t == 3) && announce < 8) {
      len = db_strlen(name);
      x = (20 - len) >> 1;
      for (i = 0; i < len; i++) {
        tiles[x + i] = (name[i] - 'A' + 1) * 2 + t - 2;
        att[x + i] = 1 | 0x08;
      }
    } else if (t == 2) {
      put_text(tiles, 3, "DATA ANALYST");
      att[3] = 1;
      for (i = 3; i < 15; i++)
        att[i] = 1;
    } else if (t == 3) {
      put_text(tiles, 6, "UNLOCKED");
    } else if (t == 5 && bonus) {
      put_text(tiles, 4, "PERFECT +");
      tiles[13] = 17 + bonus / 100;
      tiles[14] = 17 + (bonus / 10) % 10;
      tiles[15] = 17 + bonus % 10;
    }
    banner_row(r + t, tiles, att);
  }
}
static void begin_level_up(void) {
  uint8_t i;
  for (i = 0; i < MAXP; i++) {
    if (!pr_on[i])
      continue;
    pr_on[i] = 0;
    add_score(5);
    burst(pr_x[i] >> FX, pr_y[i] >> FX, 3, 3, 0);
  }
  for (i = 0; i < 5; i++)
    if (tel[i]) {
      tel[i] = 0;
      telegraph(i, 0);
    }
  bonus = perfect ? 100 * (db_level + 1) : 0;
  if (bonus)
    add_score(bonus);
  pl_phase = P_LEVEL;
  pl_timer = 0;
  dash_t = 0;
}
static void level_step(void) {
  uint16_t t = ++pl_timer;
  uint8_t next = db_level + 1;
  if (t < 24)
    db_pal_fill(1, (t & 4) ? RGB(31, 31, 31) : company_cols[db_level * 3]);
  else if (t == 24)
    company_colours(db_level);
  if (t == 12)
    music(SONG_LEVEL);
  /* Kirby-style copy: flicker between old form, white and the new form. */
  if (t >= 24 && t < 96) {
    flash_t = (t & 3) == 0 || t > 84 ? 2 : 0;
    if (t == 60)
      form_pal(pl_form = next > 7 ? 8 : next);
    if (t >= 60 && (t & 7) < 2) {
      spark((pl_x >> FX) + (sine32[(t << 1) & 31] >> 2), (pl_y >> FX) + (sine32[((t << 1) + 8) & 31] >> 2),
            0, 0, 6, SPR_SPARK_N(5), 2);
    }
  }
  if (t == 60) {
    company_colours(next > 7 ? 7 : next);
    if (next < 8) {
      hud_name(next);
      db_put(11, 5, T_MINI0 + next, 1);
    }
    db_pal(OBJ + 5, logo_pals + (next > 7 ? 7 : next) * 4);
  }
  if (t == 96) {
    burst(pl_x >> FX, pl_y >> FX, 14, 2, 1);
    chomp_t = 12;
  }
  if (t == 100)
    splash_text(next);
  if (t == 170) {
    art_stars(splash_row, (splash_row + 5) & 31);
    if (next > 7) {
      db_won = 1;
      credits_enter();
      return;
    }
    level_setup(next);
    db_pal(OBJ + 1, logo_pals + next * 4);
  }
  if (t >= 200) {
    pl_phase = P_PLAY;
    hold = 48;
  }
}
static void dying_step(void) {
  uint16_t t = ++pl_timer;
  if (t == 20)
    music(SONG_OVER);
  if (t < 60 && !(t & 7))
    burst((pl_x >> FX) + (db_rand() & 15) - 8, (pl_y >> FX) + (db_rand() & 15) - 8, 4, 2, 1);
  if (t >= 120) {
    db_scy = 0;
    results_enter();
  }
}

/* -------------------------------------------------------------- drawing */
static uint8_t rot;
static void draw(uint8_t frozen) {
  uint8_t i, k, f, tile;
  int16_t x, y;
  /* Popups first so they stay on top. */
  for (i = 0; i < 2; i++) {
    if (!pp_life[i])
      continue;
    if (!frozen && !(--pp_life[i] & 3))
      pp_y[i]--;
    for (k = 0; k < pp_n[i]; k++)
      spr8(SPR_POP_N(pp_ch[i][k]), pp_x[i] + k * 4 + ox, pp_y[i] + oy, 3 | 0x08);
  }
  i = pl_phase == P_INTRO ? 0 : db_level + 1;
  if ((pl_phase == P_LEVEL && pl_timer >= 100 && pl_timer < 170 && db_level < 7) ||
      (pl_phase == P_INTRO && pl_timer < 100)) {
    y = (int16_t)(uint8_t)(((splash_row + 2) << 3) - db_scy);
    x = ((20 - db_strlen(names[i])) << 2) - 22;
    spr16(SPR_LOGO_N(i), x, y, 5);
    spr16(SPR_LOGO_N(i), 160 - x - 16, y, 5);
  }
  if (pl_phase != P_DYING || pl_timer < 6) {
    if (!inv_t || (db_frame & 4)) {
      f = chomp_t ? 1 : (uint8_t)(db_frame >> 4) & 1;
      tile = SPR_FORM(pl_form == 8 ? 0 : pl_form, f);
      y = (pl_y >> FX) - 8 + oy + ((db_frame >> 4) & 1);
      spr16(tile, (pl_x >> FX) - 8 + ox, y, flash_t ? 4 : 0);
    }
  }
  /* Rotate the draw order so crowded lines flicker instead of vanishing. */
  rot = rot >= MAXP - 1 ? 0 : rot + 1;
  for (k = 0, i = rot; k < MAXP; k++, i = i == MAXP - 1 ? 0 : i + 1) {
    if (!pr_on[i])
      continue;
    x = (pr_x[i] >> FX) - 8 + ox;
    y = (pr_y[i] >> FX) - 8 + oy;
    spr16(SPR_LOGO_N(pr_logo[i]), x, y, 1);
  }
  for (i = 0; i < MAXK; i++) {
    if (!pk_life[i])
      continue;
    if (!frozen) {
      pk_life[i]--;
      pk_x[i] += pk_vx[i];
      pk_y[i] += pk_vy[i];
      pk_vy[i] += 4;
    }
    spr8(pk_tile[i], (pk_x[i] >> FX) - 4 + ox, (pk_y[i] >> FX) - 4 + oy, pk_pal[i]);
  }
}

static void pause_toggle(void) {
  pl_paused ^= 1;
  if (pl_paused) {
    db_fade = 4;
    db_pal_apply();
    db_text(6, 4, "  PAUSED  ", 0);
    NR51_REG = 0;
  } else {
    db_fade = 8;
    db_pal_apply();
    hud_name(pl_phase == P_LEVEL && pl_timer >= 60 && db_level < 7 ? db_level + 1 : db_level);
    NR51_REG = db_muted ? 0 : 0xFF;
  }
}

void play_step(void) DB_BANKED {
  uint16_t c;
  if ((db_pressed & J_START) && pl_phase == P_PLAY)
    pause_toggle();
  if (pl_paused) {
    if (!(db_frame & 31))
      db_text(6, 4, (db_frame & 32) ? "          " : "  PAUSED  ", 0);
    draw(1);
    return;
  }
  if (pl_phase == P_PLAY) {
    update_player();
    update_fire();
    update_projectiles();
  } else if (pl_phase == P_INTRO) {
    update_player();
    if (++pl_timer == 100) {
      art_stars(splash_row, (splash_row + 5) & 31);
      pl_phase = P_PLAY;
    }
  } else if (pl_phase == P_LEVEL) {
    level_step();
    if (db_mode != M_PLAY)
      return;
    update_projectiles();
  } else {
    dying_step();
    if (db_mode != M_PLAY)
      return;
  }
  /* Timers and feedback. */
  if (chomp_t)
    chomp_t--;
  if (flash_t)
    flash_t--;
  if (inv_t)
    inv_t--;
  if (shake_t) {
    shake_t--;
    ox = shake_t ? ((db_rand() & 2) ? 1 : -1) * (shake_t > 8 ? 2 : 1) : 0;
    oy = shake_t ? ((db_rand() & 2) ? 1 : -1) : 0;
  }
  if (white_t && !--white_t)
    db_pal_apply();
  if (miss_t) {
    miss_t--;
    tint_sky(miss_t ? RGB(8, 0, 2) : pal_stars[0]);
  }
  /* Stars drift down; faster during the transformation. */
  star_t++;
  if (pl_phase == P_LEVEL && pl_timer > 24 && pl_timer < 96)
    db_scy -= 3;
  else if (pl_phase != P_INTRO && (pl_phase != P_LEVEL || pl_timer < 100 || pl_timer > 170) && !(star_t & 3))
    db_scy--;
  db_scx = (uint8_t)ox;
  /* The base's mark breathes. */
  if (!(star_t & 7) && !tel[2] && !white_t) {
    c = (uint8_t)sine32[(star_t >> 3) & 31];
    PAL_LIVE[6 * 4 + 3] = (int8_t)c > 0 ? RGB(10, 31, 20) : pal_base[3];
    db_pal_dirty |= 1 << 6;
  }
  /* The Space Invaders heartbeat quickens as the meter fills. */
  if (pl_phase == P_PLAY && !hold && !--beat_t) {
    beat(heartbeat[beat_n++ & 3]);
    beat_t = interval - (uint8_t)(((uint16_t)interval * pl_catches) / (pl_need * 2u));
    if (beat_t < 12)
      beat_t = 12;
  } else if (!beat_t)
    beat_t = 1;
  draw(0);
  hud();
}
