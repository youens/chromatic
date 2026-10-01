/* Brand splash, title, how to play, results and credits. */
#include "dreambase.h"
static const uint8_t sine32[32] = SINE32_INIT;
static const uint8_t splash_order[] = SPLASH_ORDER_INIT;
#include <string.h>

#define RED_TEXT_1 RGB(9, 1, 3)
static const uint16_t pal_red[4] = {RGB(0, 0, 1), RGB(9, 1, 3), RGB(25, 5, 9), RGB(31, 20, 22)};
static const uint16_t pal_hidden[4] = {RGB(0, 0, 1), RGB(0, 0, 1), RGB(0, 0, 1), RGB(0, 0, 1)};
static const uint16_t pal_hot[4] = {RGB(0, 0, 1), RGB(20, 31, 26), RGB(28, 31, 30), RGB(31, 31, 31)};
static const uint16_t pal_warm[4] = {RGB(0, 0, 1), RGB(2, 18, 10), RGB(10, 30, 20), RGB(22, 31, 27)};
static const uint16_t pal_mark[4] = {RGB(0, 0, 1), RGB(0, 12, 7), RGB(2, 22, 11), RGB(2, 29, 14)};
static const uint16_t pal_dim[4] = {RGB(0, 0, 1), RGB(3, 4, 7), RGB(10, 12, 16), RGB(16, 18, 22)};
static const uint16_t pal_sparkle[4] = {RGB(0, 0, 1), RGB(9, 7, 2), RGB(31, 26, 9), RGB(31, 31, 31)};
static const char *const names[8] = {"SUPABASE", "STRIPE", "POSTHOG", "VERCEL",
                                     "GITHUB", "CLICKHOUSE", "LINEAR", "DREAMBASE"};
static const char *const roles[8] = {"THE DATABASE", "THE PAYMENTS", "THE ANALYTICS",
                                     "THE DEPLOYS", "THE CODE", "THE WAREHOUSE",
                                     "THE ISSUES", "THE DREAM"};
static const char *const modes[3] = {"   EASY   ", "  NORMAL  ", "   HARD   "};

static uint16_t st;
static uint8_t page, marchx, march_dir, march_frame, chomp[8], next_shot, last_amp;
static int8_t fx;
static uint8_t tel_slot, tel_time, comet;
static int16_t comet_x, comet_y;

static void twinkle(void) {
  uint16_t c[4];
  uint8_t k = (uint8_t)(db_frame >> 4) & 3;
  c[0] = pal_stars[0];
  c[1] = pal_stars[1 + (k & 1)];
  c[2] = pal_stars[1 + ((k + 1) % 3)];
  c[3] = pal_stars[3 - (k == 2)];
  db_pal(4, c);
}
static void clear_pools(void) {
  uint8_t i;
  for (i = 0; i < MAXP; i++)
    pr_on[i] = 0;
  for (i = 0; i < MAXK; i++)
    pk_life[i] = 0;
}
static void burst(int16_t x, int16_t y, uint8_t n, uint8_t pal) {
  uint8_t i, k = 0;
  for (i = 0; i < MAXK && n; i++) {
    if (pk_life[i])
      continue;
    n--;
    k += 5;
    pk_life[i] = 14 + (db_rand() & 7);
    pk_x[i] = x << FX;
    pk_y[i] = y << FX;
    pk_vx[i] = (int16_t)(int8_t)sine32[(k + db_rand()) & 31];
    pk_vy[i] = (int16_t)(int8_t)sine32[(k + 8 + db_rand()) & 31];
    pk_tile[i] = SPR_SPARK_N(db_rand() & 3);
    pk_pal[i] = pal;
  }
}
static void draw_sparks(void) {
  uint8_t i;
  for (i = 0; i < MAXK; i++) {
    if (!pk_life[i])
      continue;
    pk_life[i]--;
    pk_x[i] += pk_vx[i];
    pk_y[i] += pk_vy[i];
    pk_vy[i] += 3;
    spr8(pk_tile[i], (pk_x[i] >> FX) - 4, (pk_y[i] >> FX) - 4, pk_pal[i]);
  }
}

/* ============================================================ splash */
void splash_enter(void) DB_BANKED {
  uint8_t i;
  db_screen_off();
  db_mode = M_SPLASH;
  HIDE_WIN;
  db_map_hi = 0x98;
  db_clear_rows(0, MAP_ROWS - 1);
  art_sky(0, 17);
  art_splash();
  db_pal(0, pal_text);
  db_pal(1, pal_mark);
  db_pal(2, pal_hot);
  db_pal(3, pal_warm);
  db_pal(4, pal_stars);
  db_pal(5, pal_green);
  db_pal(6, pal_dim);
  db_pal(7, pal_hidden);
  db_pal(OBJ + 7, pal_sparkle);
  for (i = 0; i < 8; i++)
    db_pal(OBJ + i, pal_sparkle);
  clear_pools();
  db_screen_on();
  st = 0;
}
static void set_pal(uint8_t x, uint8_t y, uint8_t pal) {
  uint16_t i = ((uint16_t)y << 5) + x;
  ATT[i] = (ATT[i] & 0xF8) | pal;
  db_row_dirty[y] = 1;
}
void splash_step(void) DB_BANKED {
  uint8_t k, t;
  st++;
  if (st <= 16) {
    db_fade = (uint8_t)(st >> 1);
    db_pal_apply();
  }
  /* The mark sweeps in tile by tile, white-hot, then cools to green: a
     tile appears every second frame and changes palette 10 and 22 frames
     later. Entries pack (row << 3) | column. */
  if (st >= 20 && !(st & 1)) {
    k = (uint8_t)((st - 20) >> 1);
    if (k < SPLASH_TILES) {
      t = splash_order[k];
      set_pal(7 + (t & 7), 2 + (t >> 3), 2);
      if (!(k & 3))
        sfx(SFX_BLIP, k >> 1);
    }
    k = (uint8_t)((st - 20) >> 1) - 5;
    if (st >= 30 && k < SPLASH_TILES) {
      t = splash_order[k];
      set_pal(7 + (t & 7), 2 + (t >> 3), 3);
    }
    k = (uint8_t)((st - 20) >> 1) - 11;
    if (st >= 42 && k < SPLASH_TILES) {
      t = splash_order[k];
      set_pal(7 + (t & 7), 2 + (t >> 3), 1);
    }
  }
  /* The wordmark types in, column by column. */
  if (st >= 96 && st < 96 + 32 && !(st & 1)) {
    k = (uint8_t)((st - 96) >> 1);
    set_pal(2 + k, 9, 2);
    set_pal(2 + k, 10, 2);
  }
  if (st >= 104 && st < 104 + 32 && !(st & 1)) {
    k = (uint8_t)((st - 104) >> 1);
    set_pal(2 + k, 9, 0);
    set_pal(2 + k, 10, 0);
  }
  if (st == 100)
    music(SONG_LOGO);
  if (st == 140)
    db_center(13, "DREAMBASE.COM", 5);
  if (st == 160)
    db_center(16, "PRESENTS", 6);
  /* A glint on the mark and sparks around it. */
  if (st >= 140 && st < 156) {
    k = (uint8_t)(st - 140);
    db_pal_fill(1, k < 8 ? RGB(20 + k, 31, 24 + k / 2) : RGB(36 - k, 31, 32 - k));
  } else if (st == 156)
    db_pal(1, pal_mark);
  if (st >= 120 && !(st & 15) && st < 220)
    burst(56 + (db_rand() & 47), 16 + (db_rand() & 47), 3, OBJ - 8 + 7);
  draw_sparks();
  if (st >= 250 || (st > 24 && (db_pressed & (J_A | J_B | J_START))))
    title_enter();
}

/* ============================================================= title */
static void draw_mode(void) {
  db_text(4, 16, "<", 0);
  db_text(15, 16, ">", 0);
  db_text(5, 16, modes[db_diff], 0);
}
static void set_wave(uint8_t amp) {
  uint8_t i;
  if (amp == last_amp)
    return;
  last_amp = amp;
  for (i = 0; i < 32; i++)
    WAVE[i] = (int8_t)(((int16_t)(int8_t)sine32[i] * amp) >> 6);
}
void title_enter(void) DB_BANKED {
  uint8_t i;
  db_screen_off();
  db_mode = M_TITLE;
  HIDE_WIN;
  db_map_hi = 0x98;
  db_clear_rows(0, MAP_ROWS - 1);
  art_sky(0, 11);
  art_title();
  art_base(12, 4);
  for (i = 0; i < 5; i++)
    art_cannon(i, 1, 7);
  db_text(5, 0, "BEST", 3);
  db_digits(10, 0, db_best, 5, 3);
  draw_mode();
  db_center(17, "DREAMBASE.COM", 1);
  db_pal(0, pal_text);
  db_pal(1, pal_green);
  {
    uint16_t c[4];
    c[0] = pal_text[0];
    c[1] = company_cols[21];
    c[2] = RGB(3, 4, 9);
    c[3] = RGB(31, 31, 31);
    db_face = c[1];
    db_pal(2, c);
  }
  db_pal(3, pal_gold);
  db_pal(4, pal_stars);
  db_pal(5, pal_moon);
  db_pal(6, pal_base);
  {
    uint16_t c[4];
    for (i = 0; i < 4; i++)
      c[i] = pal_flash[i];
    c[3] = company_cols[0];
    db_pal(7, c);
  }
  /* Formation pairs share palettes: [_, odd body, even body, white]. */
  for (i = 0; i < 4; i++) {
    static const uint8_t pair[8] = {1, 2, 3, 5, 4, 6, 7, 8};
    uint16_t c[4];
    c[0] = 0;
    c[1] = form_pals[pair[i * 2 + 1] * 4 + 2];
    c[2] = form_pals[pair[i * 2] * 4 + 2];
    c[3] = RGB(31, 31, 31);
    db_pal(OBJ + i, c);
  }
  db_pal(OBJ + 7, pal_sparkle);
  clear_pools();
  for (i = 0; i < 8; i++)
    chomp[i] = 0;
  marchx = 12;
  march_dir = 1;
  march_frame = 0;
  next_shot = 40;
  last_amp = 255;
  set_wave(0);
  db_wave_t = db_copper_t = db_shine = 0;
  comet = 0;
  st = 0;
  db_rmode = 1;
  db_screen_on();
  music(SONG_TITLE);
  db_fade_to(8);
}
static int16_t inv_x(uint8_t i) {
  return (int16_t)marchx + 8 + (i & 3) * 32;
}
static int16_t inv_y(uint8_t i) {
  return i < 4 ? 54 : 71;
}
static const uint8_t side_slots[4] = {0, 1, 3, 4};
static const uint8_t slot_x[5] = {24, 48, 80, 112, 136};
/* A shooting star now and then, with a fading tail. */
static void shooting_star(void) {
  uint8_t k;
  if (!comet) {
    if ((db_rand() & 127) == 0) {
      comet = 48;
      comet_x = -8;
      comet_y = 2 + (db_rand() & 15);
    }
    return;
  }
  /* It flies behind the lettering, showing through the gaps. */
  comet--;
  comet_x += 4;
  comet_y += comet & 1;
  for (k = 0; k < 4; k++)
    spr8(SPR_SPARK_N(k == 0 ? 3 : k == 1 ? 2 : 0), comet_x - k * 7, comet_y - k * 2, 7 | 0x80);
}
static void title_show(void) {
  uint8_t i, f, n;
  int16_t x, y, tx;
  /* The formation marches in Space Invaders steps. */
  if (!(st & 15)) {
    march_frame ^= 1;
    if (march_dir) {
      marchx += 2;
      if (marchx >= 24)
        march_dir = 0;
    } else {
      marchx -= 2;
      if (!marchx)
        march_dir = 1;
    }
  }
  for (i = 0; i < 8; i++) {
    f = chomp[i] ? 1 : march_frame;
    if (chomp[i])
      chomp[i]--;
    x = inv_x(i);
    y = inv_y(i);
    spr16(SPR_TITLE + i * 8 + f * 4, x, y, (i >> 1) | 0x08);
  }
  /* Turrets telegraph, then fire a partner logo the invaders gulp down. */
  if (tel_time) {
    if (--tel_time == 0) {
      art_cannon(tel_slot, 1, 7);
      for (n = 0; n < 3 && pr_on[n]; n++)
        ;
      if (n < 3) {
        pr_on[n] = 1;
        pr_logo[n] = db_rand() & 7;
        pr_x[n] = slot_x[tel_slot];
        pr_y[n] = 100 << FX;
        pr_bx[n] = db_rand() & 7;
        db_pal(OBJ + 4 + n, logo_pals + pr_logo[n] * 4);
      }
    }
  } else if (!--next_shot) {
    tel_slot = side_slots[db_rand() & 3];
    tel_time = 12;
    art_cannon(tel_slot, 1, 3);
    next_shot = 34 + (db_rand() & 31);
  }
  for (n = 0; n < 3; n++) {
    if (!pr_on[n])
      continue;
    i = pr_bx[n];
    tx = inv_x(i) + 8;
    x = pr_x[n];
    if (x < tx)
      x++;
    else if (x > tx)
      x--;
    pr_x[n] = x;
    pr_y[n] -= 72;
    y = pr_y[n] >> FX;
    if (y < inv_y(i) + 12) {
      pr_on[n] = 0;
      chomp[i] = 10;
      burst(x, y, 6, 7);
      sfx(SFX_EAT, (uint8_t)((db_rand() & 3) << 1));
    } else
      spr16(SPR_LOGO_N(pr_logo[n]), x - 8, y - 8, 4 + n);
  }
  draw_sparks();
  shooting_star();
}
void title_step(void) DB_BANKED {
  uint8_t k, amp;
  st++;
  twinkle();
  /* INVADERS: the copper gradient climbs through every partner colour,
     and every few seconds the letters ripple. */
  if (!(st & 3))
    db_copper_t++;
  db_wave_t++;
  k = (uint8_t)st;
  amp = k < 64 ? sine32[(k >> 2) & 15] >> 4 : 0;
  set_wave(amp);
  /* The mark and the dome pulse together. */
  k = sine32[(uint8_t)(st >> 2) & 31];
  {
    uint16_t c[4];
    int8_t g = (int8_t)k >> 4;
    c[0] = pal_green[0];
    c[1] = pal_green[1];
    c[2] = pal_green[2];
    c[3] = RGB(2 + (g > 0 ? g * 3 : 0), 29, 14 + (g > 0 ? g * 2 : 0));
    db_pal(1, c);
    c[0] = pal_base[0];
    c[1] = pal_base[1];
    c[2] = pal_base[2];
    db_pal(6, c);
  }
  if (!(st & 63)) {
    uint16_t c[4];
    uint8_t i;
    for (i = 0; i < 4; i++)
      c[i] = pal_flash[i];
    c[3] = company_cols[((st >> 6) & 7) * 3];
    db_pal(7, c);
  }
  /* Every four seconds a shine scans down the wordmark. */
  k = (uint8_t)(st & 255);
  db_shine = k >= 200 && k < 217 ? 8 + (k - 200) : 0;
  /* The top line alternates the best score with the tagline. */
  if ((st & 255) == 0) {
    db_text(0, 0, "                    ", 3);
    if (st & 256)
      db_center(0, "EAT THE DATA", 1);
    else {
      db_text(5, 0, "BEST", 3);
      db_digits(10, 0, db_best, 5, 3);
    }
  }
  /* PRESS START: visible 0.8 s, hidden 0.4 s. */
  k = (uint8_t)(st % 72);
  if (k == 0)
    db_center(11, "PRESS START", 0);
  else if (k == 48)
    db_text(4, 11, "           ", 0);
  title_show();
  if (db_pressed & (J_LEFT | J_RIGHT)) {
    if ((db_pressed & J_LEFT) && db_diff)
      db_diff--;
    if ((db_pressed & J_RIGHT) && db_diff < 2)
      db_diff++;
    draw_mode();
    sfx(SFX_BLIP, db_diff * 3);
  }
  if (db_pressed & (J_START | J_A)) {
    music(SONG_START);
    for (k = 0; k < 40 && !db_home; k++) {
      db_center(11, (k & 4) ? "           " : "PRESS START", 0);
      title_show();
      db_wait();
    }
    play_enter();
  } else if (db_pressed & J_B) {
    sfx(SFX_SELECT, 0);
    page = 0;
    help_enter();
  }
}

/* ============================================================== help */
static void help_page(void) {
  uint8_t i;
  db_clear_rows(0, 17);
  if (page == 0) {
    db_center(0, "HOW TO PLAY", 1);
    db_text(4, 2, "YOU ARE THE", 0);
    db_text(4, 3, "DATA ANALYST.", 1);
    db_text(1, 5, "THE BASE FIRES DATA", 0);
    db_text(1, 6, "UP AT YOU. EAT IT!", 0);
    db_text(1, 8, "D-PAD", 1);
    db_text(8, 8, "MOVE", 0);
    db_text(1, 9, "A OR B", 1);
    db_text(8, 9, "DASH", 0);
    db_text(1, 10, "START", 1);
    db_text(8, 10, "PAUSE", 0);
    db_text(1, 12, "MISSES DRAIN LIFE.", 0);
    db_text(1, 13, "STREAKS X2 X4 X8!", 3);
    db_text(1, 14, "FILL THE METER AND", 0);
    db_text(1, 15, "TRANSFORM!", 1);
    db_text(0, 17, "A NEXT   B BACK", 6);
  } else {
    db_center(0, "THE DATA STACK", 1);
    for (i = 0; i < 8; i++) {
      db_text(4, 1 + i * 2, names[i], 0);
      db_text(4, 2 + i * 2, roles[i], 6);
    }
    db_text(0, 17, "START PLAY  B BACK", 6);
  }
}
void help_enter(void) DB_BANKED {
  uint8_t i;
  db_shine = 0;
  db_screen_off();
  db_mode = M_HELP;
  HIDE_WIN;
  db_map_hi = 0x98;
  db_pal(0, pal_text);
  db_pal(1, pal_green);
  db_pal(3, pal_gold);
  db_pal(6, pal_dim);
  for (i = 0; i < 8; i++)
    db_pal(OBJ + i, logo_pals + i * 4);
  db_pal(OBJ, form_pals);
  help_page();
  db_screen_on();
  db_fade_to(8);
}
void help_step(void) DB_BANKED {
  uint8_t i;
  st++;
  if (page == 0) {
    db_pal(OBJ, form_pals);
    spr16(SPR_FORM(0, (st >> 4) & 1), 8, 14 + ((st >> 3) & 1), 0);
  } else {
    db_pal(OBJ, logo_pals);
    for (i = 0; i < 8; i++)
      spr16(SPR_LOGO_N(i), 8, 8 + i * 16, i);
  }
  if ((db_pressed & (J_A | J_RIGHT)) && page == 0) {
    page = 1;
    sfx(SFX_BLIP, 7);
    help_page();
  } else if (db_pressed & (J_B | J_LEFT)) {
    sfx(SFX_BLIP, 0);
    if (page) {
      page = 0;
      help_page();
    } else
      title_enter();
  } else if (db_pressed & J_START || (page && (db_pressed & J_A))) {
    music(SONG_START);
    play_enter();
  }
}

/* ============================================================ results */
static uint8_t new_best;
void results_enter(void) DB_BANKED {
  db_screen_off();
  db_mode = M_RESULTS;
  HIDE_WIN;
  db_map_hi = 0x98;
  db_clear_rows(0, MAP_ROWS - 1);
  art_sky(0, 17);
  new_best = db_score > db_best;
  if (new_best)
    db_best = db_score;
  if (db_won) {
    db_tall(3, 1, "DATA EATEN!", 1);
    db_center(4, "ALL EIGHT STACKS", 0);
  } else {
    db_tall(5, 1, "GAME OVER", 2);
    db_text(3, 4, "REACHED", 6);
    db_text(11, 4, names[db_level], 0);
  }
  db_center(6, "FINAL SCORE", 6);
  db_tall(7, 7, "     ", 0);
  {
    /* Five tall digits for the score. */
    uint16_t v = db_score;
    static const uint16_t pw[5] = {10000, 1000, 100, 10, 1};
    char s[6];
    uint8_t i, d;
    for (i = 0; i < 5; i++) {
      d = 0;
      while (v >= pw[i]) {
        v -= pw[i];
        d++;
      }
      s[i] = '0' + d;
    }
    s[5] = 0;
    db_tall(7, 7, s, 0);
  }
  db_text(5, 10, "BEST", 3);
  db_digits(10, 10, db_best, 5, 3);
  db_center(15, "A RETRY   B TITLE", 0);
  db_center(17, "DREAMBASE.COM", 1);
  db_pal(0, pal_text);
  db_pal(1, pal_green);
  db_pal(2, pal_red);
  db_pal(3, pal_gold);
  db_pal(4, pal_stars);
  db_pal(6, pal_dim);
  db_pal(OBJ, form_pals + (db_won ? 8 : (db_level ? db_level : 0)) * 4);
  db_pal(OBJ + 7, pal_sparkle);
  clear_pools();
  db_screen_on();
  music(db_won ? SONG_LEVEL : SONG_OVER);
  db_fade_to(8);
  st = 0;
}
void results_step(void) DB_BANKED {
  uint8_t form = db_won ? 0 : db_level;
  st++;
  twinkle();
  if (new_best && !(st & 15))
    db_center(11, (st & 16) ? "NEW BEST!" : "         ", 3);
  spr16(SPR_FORM(form, (st >> 4) & 1), 72, 98 + ((st >> 3) & 1), 0);
  if (db_won && !(st & 31))
    burst(80, 104, 6, 7);
  draw_sparks();
  if (st > 30 && (db_pressed & (J_A | J_START))) {
    music(SONG_START);
    play_enter();
  } else if (st > 30 && (db_pressed & J_B))
    title_enter();
}

/* ============================================================ credits */
/* Each line: text, palette, and an invader form to show beside it (0xFF
   for none). */
typedef struct {
  const char *text;
  uint8_t pal, form;
} Line;
static const Line lines[] = {
    {"DREAMBASE", 1, 0xFF}, {"INVADERS", 0, 0xFF}, {"", 0, 0xFF},
    {"ALL DATA EATEN!", 3, 0xFF}, {"", 0, 0xFF}, {"STARRING", 6, 0xFF}, {"", 0, 0xFF},
    {"    SUPABASE", 0, 1}, {"    THE DATABASE", 6, 0xFF}, {"", 0, 0xFF},
    {"    STRIPE", 0, 2}, {"    THE PAYMENTS", 6, 0xFF}, {"", 0, 0xFF},
    {"    POSTHOG", 0, 3}, {"    THE ANALYTICS", 6, 0xFF}, {"", 0, 0xFF},
    {"    VERCEL", 0, 4}, {"    THE DEPLOYS", 6, 0xFF}, {"", 0, 0xFF},
    {"    GITHUB", 0, 5}, {"    THE CODE", 6, 0xFF}, {"", 0, 0xFF},
    {"    CLICKHOUSE", 0, 6}, {"    THE WAREHOUSE", 6, 0xFF}, {"", 0, 0xFF},
    {"    LINEAR", 0, 7}, {"    THE ISSUES", 6, 0xFF}, {"", 0, 0xFF},
    {"    DREAMBASE", 1, 8}, {"    THE DREAM", 6, 0xFF}, {"", 0, 0xFF}, {"", 0, 0xFF},
    {"YOUR SCORE", 6, 0xFF}, {"#", 3, 0xFF}, {"", 0, 0xFF}, {"", 0, 0xFF},
    {"THANK YOU", 0, 0xFF}, {"FOR PLAYING", 0, 0xFF}, {"", 0, 0xFF}, {"", 0, 0xFF},
    {"UNLOCK YOUR", 1, 0xFF}, {"DATA ANALYST", 1, 0xFF}, {"AT DREAMBASE.COM", 0, 0xFF},
};
#define LINES (sizeof(lines) / sizeof(lines[0]))
/* Line j sits on logical row j + 18, just below the first screen; rows are
   written one ahead of the scroll into the 32-row map. */
static uint16_t scroll, written;
static void credit_row(uint16_t logical) {
  uint8_t tiles[20], att[20], i, x;
  uint16_t n = logical - 18;
  const char *s;
  char score[6];
  for (i = 0; i < 20; i++)
    tiles[i] = att[i] = 0;
  if (logical >= 18 && n < LINES) {
    s = lines[n].text;
    if (s[0] == '#') {
      uint16_t v = db_score;
      static const uint16_t pw[5] = {10000, 1000, 100, 10, 1};
      uint8_t d;
      for (i = 0; i < 5; i++) {
        for (d = 0; v >= pw[i]; d++)
          v -= pw[i];
        score[i] = '0' + d;
      }
      score[5] = 0;
      s = score;
    }
    x = s[0] == ' ' ? 0 : (20 - db_strlen(s)) >> 1;
    for (i = 0; s[i] && x < 20; i++, x++) {
      tiles[x] = s[i] == ' ' ? 0 : s[i] - 31;
      att[x] = lines[n].pal;
    }
  }
  VBK_REG = 1;
  set_bkg_tiles(0, (uint8_t)logical & 31, 20, 1, att);
  VBK_REG = 0;
  set_bkg_tiles(0, (uint8_t)logical & 31, 20, 1, tiles);
}
void credits_enter(void) DB_BANKED {
  uint8_t i;
  db_screen_off();
  db_mode = M_CREDITS;
  HIDE_WIN;
  db_map_hi = 0x98;
  db_clear_rows(0, MAP_ROWS - 1);
  VBK_REG = 1;
  memset((uint8_t *)0x9800, 0, 1024);
  VBK_REG = 0;
  memset((uint8_t *)0x9800, 0, 1024);
  for (i = 0; i < MAP_ROWS; i++)
    db_row_dirty[i] = 0;
  scroll = 0;
  written = 18;
  db_scy = 0;
  db_pal(0, pal_text);
  db_pal(1, pal_green);
  db_pal(3, pal_gold);
  db_pal(6, pal_dim);
  for (i = 0; i < 8; i++)
    db_pal(OBJ + i, form_pals + (i + 1) * 4);
  db_pal_apply();
  for (i = 0; i < MAP_ROWS; i++)
    db_row_dirty[i] = 0;
  SCY_REG = 0;
  DISPLAY_ON;
  music(SONG_CREDITS);
  db_fade_to(8);
  st = 0;
}
void credits_step(void) DB_BANKED {
  uint8_t i;
  int16_t y;
  st++;
  if ((st & 1) && (scroll >> 3) < LINES + 9)
    scroll++;
  db_scy = (uint8_t)scroll;
  while (written <= (scroll >> 3) + 19)
    credit_row(written++);
  /* Invader sprites ride beside their company's line. */
  for (i = 0; i < LINES; i++) {
    if (lines[i].form == 0xFF)
      continue;
    y = (int16_t)(i + 18) * 8 - (int16_t)scroll;
    if (y > -16 && y < 144)
      spr16(SPR_FORM(lines[i].form == 8 ? 0 : lines[i].form, (st >> 4) & 1), 12, y - 4,
            lines[i].form - 1);
  }
  if (((scroll >> 3) >= LINES + 9 && st > 1800) || (st > 180 && (db_pressed & (J_A | J_START | J_B)))) {
    db_scy = 0;
    results_enter();
  }
}
