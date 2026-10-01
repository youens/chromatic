#ifndef ARCADE
#pragma bank 3
#endif
#include "dotwing.h"
#include "art_ids.h"

static uint8_t fire_clock, spawn_clock, boss_clock, boss_direction, turbo;
static uint8_t fx_life[4];
static int16_t fx_x[4], fx_y[4];
static uint16_t hud_score, hud_tokens;
static uint8_t hud_hp, hud_energy, hud_boss, hud_power;

static uint16_t gap(int16_t a, int16_t b) { return a > b ? a - b : b - a; }

static void effect(int16_t x, int16_t y) {
  uint8_t i;
  for (i = 0; i < 4; ++i) if (!fx_life[i]) {
    fx_life[i] = 18; fx_x[i] = x; fx_y[i] = y; return;
  }
}

static void award_tokens(uint8_t n) {
  if (dw_run_tokens <= 65535u - n) dw_run_tokens += n;
  dw_dirty = 1;
}

static void drop(int16_t x, int16_t y, uint8_t kind) {
  uint8_t i;
  for (i = 0; i < DW_DROPS; ++i) if (!dw_drops[i].active) {
    dw_drops[i].x = x; dw_drops[i].y = y;
    dw_drops[i].kind = kind; dw_drops[i].active = 1;
    return;
  }
}

static void pop(uint8_t i) {
  DwEnemy *e = &dw_enemies[i];
  uint8_t reward = 0;
  e->active = 0;
  effect(e->x + 4, e->y);
  ++dw_kills;
  if (dw_combo < 12) ++dw_combo;
  dw_points(20u + (uint16_t)dw_combo * 5u);
  award_tokens(dw_burst ? 3 : 1);
  if ((dw_kills % 13u) == 0) reward = 2;
  else if ((dw_kills % 7u) == 0) reward = 1;
  else if ((dw_kills % 5u) == 0) reward = 3;
  drop(e->x + 4, e->y, reward);
  dw_noise(0x23);
}

static void hurt(void) {
  if (dw_invincible || dw_burst) return;
  if (dw_hp) --dw_hp;
  dw_invincible = 100;
  dw_combo = 0;
  if (dw_power) --dw_power;
  dw_dirty = 1;
  dw_noise(0x64);
  if (!dw_hp) dw_finish(0);
}

static void bullet(int16_t x, int16_t y, int8_t vx, int8_t vy) {
  uint8_t i;
  for (i = 0; i < DW_BULLETS; ++i) if (!dw_bullets[i].active) {
    dw_bullets[i].x = x; dw_bullets[i].y = y;
    dw_bullets[i].vx = vx; dw_bullets[i].vy = vy;
    dw_bullets[i].active = 1; return;
  }
}

static void laser(int16_t x, int8_t vx) {
  uint8_t i;
  for (i = 0; i < DW_SHOTS; ++i) if (!dw_shots[i].active) {
    dw_shots[i].x = x; dw_shots[i].y = dw_py - 12;
    dw_shots[i].vx = vx; dw_shots[i].vy = -5;
    dw_shots[i].active = 1; return;
  }
}

static void fire(void) {
  uint8_t interval = 12u - dw_profile.weapon * 2u;
  if (turbo) interval = 4;
  if (fire_clock) { --fire_clock; return; }
  fire_clock = interval;
  if (dw_power || dw_profile.weapon >= 2) {
    laser(dw_px + 2, 0); laser(dw_px + 10, 0);
  } else laser(dw_px + 6, 0);
  if (dw_power >= 2 || dw_profile.weapon == 3) {
    laser(dw_px - 2, -1); laser(dw_px + 14, 1);
  }
  if (!(dw_frame & 15u)) dw_tone(1929, 0x31);
}

static void spawn(void) {
  uint8_t i;
  if (spawn_clock) { --spawn_clock; return; }
  spawn_clock = 49u - dw_sector * 4u;
  for (i = 0; i < DW_ENEMIES; ++i) if (!dw_enemies[i].active) {
    DwEnemy *e = &dw_enemies[i];
    e->x = 12u + (dw_random() % 122u);
    e->y = -16;
    e->kind = dw_sector - 1;
    /* Other labs occasionally raid the current lab's sector. */
    if ((dw_kills & 7u) == 6u) e->kind = (dw_sector + 1u) & 3u;
    e->vx = (i & 1u) ? 1 : -1;
    e->vy = (dw_sector == 4 || (i & 1u)) ? 2 : 1;
    e->hp = 1u + (dw_sector >> 1);
    e->age = 0; e->active = 1;
    return;
  }
}

static void enemies(void) {
  uint8_t i;
  for (i = 0; i < DW_ENEMIES; ++i) if (dw_enemies[i].active) {
    DwEnemy *e = &dw_enemies[i];
    ++e->age;
    e->y += e->vy;
    if (e->kind == 0 || e->kind == 2) {
      if (!(e->age & 15u)) e->vx = -e->vx;
      e->x += e->vx;
    } else if (e->kind == 3 && !(e->age & 1u)) e->x += e->vx;
    if (e->x < 4 || e->x > 140) e->vx = -e->vx;
    if (e->age == 35 || (dw_sector > 2 && e->age == 61)) {
      int8_t aim = dw_px > e->x + 16 ? 1 : dw_px + 16 < e->x ? -1 : 0;
      bullet(e->x + 4, e->y + 8, aim, 2);
      if (e->kind == 1) bullet(e->x + 8, e->y + 8, -aim, 2);
    }
    if (e->y > 132) { e->active = 0; dw_combo = 0; }
    else if (gap(e->x, dw_px) < 11 && gap(e->y, dw_py) < 11) {
      e->active = 0; effect(e->x, e->y); hurt();
      if (dw_state != DW_FLIGHT) return;
    }
  }
}

static void boss(void) {
  uint8_t i;
  if (!dw_boss_active) {
    if (dw_stage_frame < 1800) return;
    dw_boss_active = 1;
    dw_boss_max = dw_boss_hp = 54u + dw_sector * 14u;
    dw_boss_x = 64; dw_boss_y = -32;
    boss_clock = 50; boss_direction = 0;
    for (i = 0; i < DW_ENEMIES; ++i) dw_enemies[i].active = 0;
    for (i = 0; i < DW_BULLETS; ++i) dw_bullets[i].active = 0;
    dw_dirty = 1;
    dw_tone(1540, 0x94);
  }
  if (dw_boss_y < 20) { ++dw_boss_y; return; }
  if (!(dw_frame & 1u)) dw_boss_x += boss_direction ? -1 : 1;
  if (dw_boss_x >= 114) boss_direction = 1;
  else if (dw_boss_x <= 14) boss_direction = 0;
  if (boss_clock) --boss_clock;
  else {
    boss_clock = 48u - dw_sector * 4u;
    if (dw_sector == 1) {
      bullet(dw_boss_x + 4, dw_boss_y + 23, -1, 2);
      bullet(dw_boss_x + 14, dw_boss_y + 27, 0, 3);
      bullet(dw_boss_x + 24, dw_boss_y + 23, 1, 2);
    } else if (dw_sector == 2) {
      bullet(dw_boss_x + 14, dw_boss_y + 24, -2, 2);
      bullet(dw_boss_x + 14, dw_boss_y + 24, -1, 3);
      bullet(dw_boss_x + 14, dw_boss_y + 24, 1, 3);
      bullet(dw_boss_x + 14, dw_boss_y + 24, 2, 2);
    } else if (dw_sector == 3) {
      for (i = 0; i < 4; ++i)
        bullet(dw_boss_x + i * 8, dw_boss_y + 28, (i & 1u) ? 1 : -1, 3);
    } else {
      int8_t aim = dw_px > dw_boss_x + 24 ? 1 : dw_px + 8 < dw_boss_x ? -1 : 0;
      bullet(dw_boss_x + 2, dw_boss_y + 24, aim - 1, 3);
      bullet(dw_boss_x + 14, dw_boss_y + 28, aim, 3);
      bullet(dw_boss_x + 26, dw_boss_y + 24, aim + 1, 3);
    }
  }
  if (gap(dw_px + 8, dw_boss_x + 16) < 22 && gap(dw_py + 8, dw_boss_y + 16) < 22)
    hurt();
}

static void projectiles(void) {
  uint8_t i, j, damage = 1u + dw_profile.weapon;
  for (i = 0; i < DW_SHOTS; ++i) if (dw_shots[i].active) {
    DwBullet *s = &dw_shots[i];
    s->x += s->vx; s->y += s->vy;
    if (s->y < -16 || s->x < -8 || s->x > 160) { s->active = 0; continue; }
    if (dw_boss_active == 1 && dw_boss_y >= 0 &&
        gap(s->x + 4, dw_boss_x + 16) < 19 && gap(s->y + 8, dw_boss_y + 16) < 21) {
      s->active = 0;
      if (dw_boss_hp > damage) dw_boss_hp -= damage;
      else { dw_boss_hp = 0; dw_boss_active = 2; }
      dw_dirty = 1;
      effect(s->x, s->y);
      continue;
    }
    for (j = 0; j < DW_ENEMIES; ++j) {
      DwEnemy *e = &dw_enemies[j];
      if (e->active && gap(s->x + 4, e->x + 8) < 10 && gap(s->y + 8, e->y + 8) < 12) {
        s->active = 0;
        if (e->hp > damage) e->hp -= damage;
        else pop(j);
        break;
      }
    }
  }
  for (i = 0; i < DW_BULLETS; ++i) if (dw_bullets[i].active) {
    DwBullet *b = &dw_bullets[i];
    b->x += b->vx; b->y += b->vy;
    if (b->y > 128 || b->y < -16 || b->x < -8 || b->x > 160) b->active = 0;
    else if (gap(b->x + 4, dw_px + 8) < 5 && gap(b->y + 8, dw_py + 8) < 6) {
      b->active = 0; hurt();
      if (dw_state != DW_FLIGHT) return;
    }
  }
}

static void pickups(void) {
  uint8_t i;
  for (i = 0; i < DW_DROPS; ++i) if (dw_drops[i].active) {
    DwDrop *d = &dw_drops[i];
    uint8_t reach = 18u + dw_profile.reactor * 8u;
    if (gap(d->x + 4, dw_px + 8) < reach && gap(d->y + 8, dw_py + 8) < reach) {
      if (d->x < dw_px + 4) ++d->x; else if (d->x > dw_px + 4) --d->x;
      if (d->y < dw_py) d->y += 2; else if (d->y > dw_py) d->y -= 2;
    } else if (!(dw_frame & 1u)) ++d->y;
    if (d->y > 132) d->active = 0;
    else if (gap(d->x + 4, dw_px + 8) < 12 && gap(d->y + 8, dw_py + 8) < 12) {
      d->active = 0;
      if (!d->kind) { award_tokens(dw_burst ? 12 : 6); dw_points(50); }
      else if (d->kind == 1) { if (dw_power < 2) ++dw_power; award_tokens(2); }
      else if (d->kind == 2) { if (dw_hp < 3u + dw_profile.shield) ++dw_hp; }
      else { turbo = 240; dw_energy = 100; award_tokens(4); }
      dw_dirty = 1;
      dw_tone(1860, 0x82);
    }
  }
}

void dw_update(void) BANKED {
  uint8_t i, speed;
  ++dw_stage_frame;
  if (dw_stage_frame == 1) {
    fire_clock = spawn_clock = 0;
    turbo = 0;
    for (i = 0; i < 4; ++i) fx_life[i] = 0;
  }
  dw_focus = (dw_keys & J_A) != 0;
  speed = dw_focus ? 1 : 2;
  if ((dw_keys & (J_LEFT | J_RIGHT)) && (dw_keys & (J_UP | J_DOWN)) && !dw_focus)
    speed = (dw_frame & 1u) ? 1 : 2;
  if (dw_keys & J_LEFT) dw_px -= speed;
  if (dw_keys & J_RIGHT) dw_px += speed;
  if (dw_keys & J_UP) dw_py -= speed;
  if (dw_keys & J_DOWN) dw_py += speed;
  if (dw_px < 2) dw_px = 2;
  else if (dw_px > 142) dw_px = 142;
  if (dw_py < 12) dw_py = 12;
  else if (dw_py > 108) dw_py = 108;
  if (dw_invincible) --dw_invincible;
  if (dw_burst) --dw_burst;
  if (turbo) --turbo;
  if (dw_energy < 100 &&
      (dw_profile.reactor >= 2 ? !(dw_frame & 1u) : !(dw_frame & 3u))) {
    uint8_t gain = (dw_profile.reactor & 1u) ? 2 : 1;
    dw_energy = dw_energy + gain > 100 ? 100 : dw_energy + gain;
  }
  if ((dw_pressed & J_B) && dw_energy >= 50) {
    dw_energy -= 50; dw_burst = 30; dw_invincible = 45;
    for (i = 0; i < DW_BULLETS; ++i) dw_bullets[i].active = 0;
    for (i = 0; i < DW_ENEMIES; ++i) if (dw_enemies[i].active) pop(i);
    if (dw_boss_active == 1) {
      uint8_t damage = 12u + dw_profile.reactor * 4u;
      if (dw_boss_hp > damage) dw_boss_hp -= damage;
      else { dw_boss_hp = 0; dw_boss_active = 2; }
    }
    dw_dirty = 1;
    dw_noise(0x45);
  }
  if (dw_boss_active < 2) {
    fire();
    if (!dw_boss_active) spawn();
    enemies();
    if (dw_state != DW_FLIGHT) return;
    boss();
    if (dw_state != DW_FLIGHT) return;
    projectiles();
    if (dw_state != DW_FLIGHT) return;
    pickups();
  }
  for (i = 0; i < 4; ++i) if (fx_life[i]) --fx_life[i];
}

void dw_draw(void) BANKED {
  uint8_t i, j;
  dw_hide();
  SCY_REG = (uint8_t)(0u - (dw_frame >> 1));
  if (!dw_invincible || !(dw_invincible & 4u) || dw_state == DW_TAKEOFF)
    dw_actor(0, DW_SPR_PLANE, dw_px, dw_py, 0);
  /* A tiny bright focus reticle makes the small collision core readable. */
  if (dw_focus) dw_sprite(2, DW_SPR_BURST, dw_px + 4, dw_py, 5);
  else if (dw_frame & 2u) dw_sprite(3, DW_SPR_BURST, dw_px + 4, dw_py + 10, 6);
  if (dw_boss_active == 1) {
    for (i = 0; i < 4; ++i) for (j = 0; j < 2; ++j)
      dw_sprite(4u + i * 2u + j, DW_SPR_BOSS + i * 4u + j * 2u,
                dw_boss_x + i * 8, dw_boss_y + j * 16, dw_sector);
  } else {
    for (i = 0; i < DW_ENEMIES; ++i) if (dw_enemies[i].active)
      dw_actor(4u + i * 2u, DW_SPR_GROK + dw_enemies[i].kind * 4u,
               dw_enemies[i].x, dw_enemies[i].y, dw_enemies[i].kind + 1u);
  }
  for (i = 0; i < DW_SHOTS; ++i) if (dw_shots[i].active)
    dw_sprite(16u + i, DW_SPR_LASER, dw_shots[i].x, dw_shots[i].y, 5);
  for (i = 0; i < DW_BULLETS; ++i) if (dw_bullets[i].active)
    dw_sprite(24u + i, DW_SPR_BULLET, dw_bullets[i].x, dw_bullets[i].y, dw_sector);
  for (i = 0; i < DW_DROPS; ++i) if (dw_drops[i].active)
    dw_sprite(32u + i, DW_SPR_TOKEN + dw_drops[i].kind * 2u,
              dw_drops[i].x, dw_drops[i].y, dw_drops[i].kind == 2 ? 2 : 6);
  for (i = 0; i < 4; ++i) if (fx_life[i])
    dw_sprite(36u + i, DW_SPR_EXPLOSION, fx_x[i], fx_y[i], 7);
  if (dw_burst) {
    uint8_t r = (30u - dw_burst) * 2u;
    dw_sprite(36, DW_SPR_EXPLOSION, dw_px - r, dw_py - r / 2, 5);
    dw_sprite(37, DW_SPR_EXPLOSION, dw_px + r, dw_py - r / 2, 5);
    dw_sprite(38, DW_SPR_EXPLOSION, dw_px - r, dw_py + r / 2, 5);
    dw_sprite(39, DW_SPR_EXPLOSION, dw_px + r, dw_py + r / 2, 5);
  }
}

static void win_label(uint8_t x, uint8_t y, const char *s, uint8_t p) {
  while (*s && x < 20) {
    VBK_REG = 0; set_win_tile_xy(x, y, (uint8_t)*s++ - 31u);
    VBK_REG = 1; set_win_tile_xy(x++, y, p);
  }
  VBK_REG = 0;
}

static void win_number(uint8_t x, uint8_t y, uint16_t n, uint8_t width, uint8_t p) {
  uint8_t i, digit;
  uint16_t place = width == 5 ? 10000 : width == 4 ? 1000 : width == 3 ? 100 : 10;
  for (i = 0; i < width; ++i) {
    digit = 0;
    while (n >= place) { n -= place; ++digit; }
    VBK_REG = 0; set_win_tile_xy(x + i, y, 17u + digit);
    VBK_REG = 1; set_win_tile_xy(x + i, y, p);
    place /= 10;
  }
  VBK_REG = 0;
}

void dw_hud(void) BANKED {
  uint8_t i;
  if (dw_dirty) {
    VBK_REG = 0; fill_win_rect(0, 0, 20, 2, 0);
    VBK_REG = 1; fill_win_rect(0, 0, 20, 2, 0); VBK_REG = 0;
    win_label(0, 0, "S", 4); win_label(7, 0, "T", 6);
    win_label(14, 0, "LAB", 5); win_number(18, 0, dw_sector, 2, 4);
    hud_score = hud_tokens = 65535u;
    hud_hp = hud_energy = hud_boss = hud_power = 255;
    dw_dirty = 0;
  }
  if (hud_score != dw_score) { win_number(1, 0, dw_score, 5, 7); hud_score = dw_score; }
  if (hud_tokens != dw_run_tokens) {
    win_number(8, 0, dw_run_tokens > 9999 ? 9999 : dw_run_tokens, 4, 6);
    hud_tokens = dw_run_tokens;
  }
  if (hud_hp != dw_hp) {
    for (i = 0; i < 6; ++i) {
      VBK_REG = 0; set_win_tile_xy(i, 1, i < dw_hp ? DW_BG_HEART : 0);
      VBK_REG = 1; set_win_tile_xy(i, 1, 2);
    }
    VBK_REG = 0; hud_hp = dw_hp;
  }
  if (hud_energy != dw_energy / 10u) {
    win_label(7, 1, "B", 4);
    for (i = 0; i < 5; ++i) {
      VBK_REG = 0; set_win_tile_xy(8 + i, 1, dw_energy >= (i + 1u) * 20u ? 30 : 14);
      VBK_REG = 1; set_win_tile_xy(8 + i, 1, dw_energy >= 50 ? 4 : 5);
    }
    VBK_REG = 0; hud_energy = dw_energy / 10u;
  }
  if (dw_boss_active == 1) {
    uint8_t bars = ((uint16_t)dw_boss_hp * 6u + dw_boss_max - 1u) / dw_boss_max;
    if (hud_boss != bars) {
      for (i = 0; i < 6; ++i) {
        VBK_REG = 0; set_win_tile_xy(14 + i, 1, i < bars ? 30 : 14);
        VBK_REG = 1; set_win_tile_xy(14 + i, 1, dw_sector == 1 ? 7 : dw_sector);
      }
      VBK_REG = 0; hud_boss = bars;
    }
  } else if (hud_power != dw_power) {
    win_label(14, 1, "PWR", 5); win_number(18, 1, 1u + dw_power + dw_profile.weapon, 2, 6);
    hud_power = dw_power;
  }
  WX_REG = 7; WY_REG = 128; SHOW_WIN;
}
