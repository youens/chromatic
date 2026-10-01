/* Flight: scrolling sky, scripted rival waves, bullets, pickups, the HUD
   and the background-layer bosses. Logic runs once per display frame. */
#ifndef ARCADE
#pragma bank 3
#endif
#include "dotwing.h"
#include <string.h>

enum { P_INTRO, P_WAVES, P_WARN, P_BOSS_IN, P_BOSS, P_BOSS_DIE, P_CLEAR, P_DEAD };
enum { K_DRONE, K_GUNNER, K_HEAVY };
enum { M_DIVE, M_SWOOP_L, M_SWOOP_R, M_SNAKE, M_HOVER, M_SIDE_L, M_SIDE_R };
enum { B_SMALL, B_BIG, B_NEEDLE, B_STAR };
enum { S_TWIN, S_SOLO, S_LEFT, S_RIGHT };
enum { D_COIN, D_POWER, D_REPAIR, D_BOLT, D_ALLY };
enum { F_BOOM, F_SPARK };

static const int8_t sin64[64] = {0,6,12,18,24,30,35,40,45,49,52,56,58,60,62,63,63,63,62,60,58,56,52,49,45,40,35,30,24,18,12,6,0,-6,-12,-18,-24,-30,-35,-40,-45,-49,-52,-56,-58,-60,-62,-63,-63,-63,-62,-60,-58,-56,-52,-49,-45,-40,-35,-30,-24,-18,-12,-6};
static const int8_t dir_x[32] = {64,63,59,53,45,36,24,12,0,-12,-24,-36,-45,-53,-59,-63,-64,-63,-59,-53,-45,-36,-24,-12,0,12,24,36,45,53,59,63};
static const int8_t dir_y[32] = {0,12,24,36,45,53,59,63,64,63,59,53,45,36,24,12,0,-12,-24,-36,-45,-53,-59,-63,-64,-63,-59,-53,-45,-36,-24,-12};
static const uint8_t atan_tab[17] = {0,0,1,1,1,2,2,2,2,3,3,3,3,3,4,4,4};

static const char * const sector_names[4] = {"AZURE ISLES", "CORAL DRIFT", "CRYSTAL CITY", "VIOLET NEXUS"};
static const char * const lab_names[4] = {"VS GROK", "VS CLAUDE", "VS GEMINI", "VS MUSE"};

uint8_t dw_phase;
uint16_t dw_cam, dw_phase_t;

/* The core's offset inside the 96x56 boss art. */
static const uint8_t core_x[4] = {40, 40, 40, 40};
static const uint8_t core_y[4] = {20, 20, 20, 30};

/* ------------------------------------------------------------- run state */
static uint8_t fire_clock, bank_t, hit_cool, chain_t, chain;
static uint8_t trail_i, run_allies, death_t, flash_t, ring_t, aim_turn;
static int8_t cam_x, bank_dir;
static uint16_t next_row, turbo;
#define bx dw_boss_x
#define by dw_boss_y
static uint16_t hud_score, hud_tokens;
static uint8_t hud_hp, hud_hpmax, hud_energy, hud_chain, hud_power, hud_boss, hud_mode, hud_row;

#define SECTOR_IDX ((uint8_t)(dw_sector - 1u) & 3u)

/* ----------------------------------------------------------- utilities */
/* Sprites are emitted straight into the frame's OAM buffer. Coordinates
   are OAM coordinates (screen + 8, + 16); anything off screen wraps to a
   large byte and is rejected by one comparison per axis. */
uint8_t clip_oy = 160;

/* ox in A, oy in E, tile and attr on the stack (popped here). */
static void spr(uint8_t ox, uint8_t oy, uint8_t tile, uint8_t attr) __naked {
  (void)ox; (void)oy; (void)tile; (void)attr;
  __asm
    ld d, a
    dec a
    cp #167
    jr nc, 9$
    ld a, (_clip_oy)
    dec a
    ld c, a
    ld a, e
    dec a
    cp c
    jr nc, 9$
    ld a, (_dw_oam_n)
    cp #40
    jr nc, 9$
    inc a
    ld (_dw_oam_n), a
    dec a
    add a, a
    add a, a
    ld c, a
    ld b, #0
    ld hl, #_dw_oam
    ld a, (hl+)
    ld h, (hl)
    ld l, a
    add hl, bc
    ld (hl), e
    inc hl
    ld (hl), d
    inc hl
    ld b, h
    ld c, l
    ldhl sp, #2
    ld a, (hl+)
    ld (bc), a
    inc bc
    ld a, (hl)
    ld (bc), a
  9$:
    pop hl
    pop af
    jp (hl)
  __endasm;
}

/* A 16-pixel-wide object from two 8x16 halves; X-flip swaps the halves.
   ox in A, oy in E, tile and attr on the stack (popped here). */
static void spr16(uint8_t ox, uint8_t oy, uint8_t tile, uint8_t attr) __naked {
  (void)ox; (void)oy; (void)tile; (void)attr;
  __asm
    ld d, a
    ld a, (_clip_oy)
    dec a
    ld c, a
    ld a, e
    dec a
    cp c
    jr nc, 9$
    ldhl sp, #2
    ld a, (hl+)
    ld b, a
    ld a, (hl)
    ld c, a
    and #0x20
    jr z, 1$
    inc b
    inc b
  1$:
    call 5$
    ld a, c
    and #0x20
    jr z, 2$
    dec b
    dec b
    dec b
    dec b
  2$:
    inc b
    inc b
    ld a, d
    add a, #8
    ld d, a
    call 5$
  9$:
    pop hl
    pop af
    jp (hl)
  5$:
    ld a, d
    dec a
    cp #167
    ret nc
    ld a, (_dw_oam_n)
    cp #40
    ret nc
    inc a
    ld (_dw_oam_n), a
    dec a
    add a, a
    add a, a
    push bc
    ld c, a
    ld b, #0
    ld hl, #_dw_oam
    ld a, (hl+)
    ld h, (hl)
    ld l, a
    add hl, bc
    pop bc
    ld (hl), e
    inc hl
    ld (hl), d
    inc hl
    ld (hl), b
    inc hl
    ld (hl), c
    ret
  __endasm;
}

/* Screen pixel (int16) to OAM byte, clamped so far-off values stay hidden. */
static uint8_t ox_of(int16_t x) {
  return (x < -8 || x > 160) ? 0 : (uint8_t)(x + 8);
}

static uint8_t oy_of(int16_t y) {
  return (y < -16 || y > 144) ? 0 : (uint8_t)(y + 16);
}

static uint8_t gap(int16_t a, int16_t b) {
  int16_t d = a - b;
  if (d < 0) d = -d;
  return d > 255 ? 255 : (uint8_t)d;
}

/* Text written into the visible background (used over open sky). */
static void sky_text(uint8_t col, uint8_t row, const char *s) {
  uint8_t len = (uint8_t)strlen(s);
  uint8_t mx = (uint8_t)((dw_scx >> 3) + col) & 31u;
  uint8_t my = (uint8_t)((dw_scy >> 3) + row) & 31u;
  uint8_t first = len;
  if (mx + len > 32) first = 32u - mx;
  VBK_REG = 1;
  fill_bkg_rect(mx, my, first, 1, PAL_TEXT);
  if (first < len) fill_bkg_rect(0, my, len - first, 1, PAL_TEXT);
  VBK_REG = 0;
  set_bkg_tiles(mx, my, first, 1, (const uint8_t *)s);
  if (first < len) set_bkg_tiles(0, my, len - first, 1, (const uint8_t *)s + first);
}

/* -------------------------------------------------------------- spawning */
/* Effects take screen pixels; they are stored biased. */
static void boom(int16_t x, int16_t y, uint8_t kind) {
  uint8_t i;
  DwFx *f = dw_fx;
  for (i = 0; i < DW_FXS; ++i, ++f) if (!f->t) {
    f->x = (uint8_t)(x + BIAS);
    f->y = (uint8_t)(y + BIAS);
    f->kind = kind;
    f->t = kind == F_BOOM ? 18 : 8;
    return;
  }
}

static void drop(int16_t x, int16_t y, uint8_t kind) {
  uint8_t i;
  DwDrop *d = dw_drops;
  if (x < -16 || x > 176 || y < -16 || y > 140) return;
  for (i = 0; i < DW_DROPS; ++i, ++d) if (!d->on) {
    d->x = POS(x);
    d->y = POS(y);
    d->kind = kind;
    d->t = (uint8_t)dw_random();
    d->on = 1;
    return;
  }
}

extern uint8_t foe_draw[DW_FOES * 5];

/* ---------------------------------------------------------------- foes */
static void kill_foe(DwFoe *e) {
  int16_t x = (int16_t)HI(e->x) - BIAS, y = (int16_t)HI(e->y) - BIAS;
  uint8_t mult, kind = e->kind;
  static const uint8_t worth[3] = {10, 30, 120};
  e->hp = 0;
  foe_draw[(uint8_t)(e - dw_foes) * 5u + 4] = 0;
  ++dw_kills;
  if (chain < 31) ++chain;
  chain_t = 80;
  mult = 1u + (chain >> 2);
  if (mult > 8) mult = 8;
  dw_points((uint16_t)worth[kind] * mult);
  if (kind == K_HEAVY) {
    boom(x + 8, y + 8, F_BOOM);
    boom(x + 24, y + 8, F_BOOM);
    boom(x + 16, y + 2, F_SPARK);
    drop(x + 12, y + 4, (dw_kills & 1u) || dw_allies >= 2 ? D_POWER : D_ALLY);
    drop(x + 20, y + 8, D_COIN);
    dw_shake = 10;
    dw_sfx(SFX_BOOM);
  } else {
    boom(x + 8, y + 8, F_BOOM);
    if (kind == K_GUNNER || !(dw_random() & 1u)) drop(x + 4, y + 4, D_COIN);
    if (!(dw_kills % 23u)) drop(x + 4, y + 8, D_REPAIR);
    else if (!(dw_kills % 17u)) drop(x + 4, y + 8, D_BOLT);
    else if (!(dw_kills % 29u)) drop(x + 4, y + 8, D_POWER);
    dw_sfx(SFX_POP);
  }
}

/* --------------------------------------------------------------- player */
static void hurt(void) {
  if (dw_invincible || dw_burst || dw_phase == P_DEAD) return;
  if (dw_hp) --dw_hp;
  dw_invincible = 120;
  chain = 0;
  if (dw_power) --dw_power;
  dw_shake = 12;
  dw_sfx(SFX_HURT);
  boom(dw_px + 8, dw_py + 7, F_SPARK);
  if (!dw_hp) {
    dw_phase = P_DEAD;
    death_t = 0;
    dw_sfx(SFX_BIGBOOM);
    boom(dw_px + 8, dw_py + 8, F_BOOM);
  }
}

static uint8_t shot_hint;
static const uint8_t shot_tiles[4] = {DW_S_SHOT, DW_S_SHOT_SOLO, DW_S_SHOT_L, DW_S_SHOT_R};

static void shot(int16_t x, int16_t y, int16_t vx, uint8_t kind) {
  uint8_t i = shot_hint, n;
  DwShot *s;
  for (n = DW_SHOTS; n; --n) {
    s = &dw_shots[i];
    if (++i == DW_SHOTS) i = 0;
    if (s->on) continue;
    shot_hint = i;
    s->x = POS(x);
    s->y = POS(y);
    s->vx = vx;
    s->vy = -1600;
    s->kind = shot_tiles[kind];
    s->on = 1;
    return;
  }
}

static uint8_t fire_level(void) {
  uint8_t l = dw_profile.weapon + dw_power;
  return l > 4 ? 4 : l;
}

static void fire(void) {
  uint8_t level = fire_level(), i;
  int16_t x = dw_px + 8, y = dw_py;
  if (fire_clock) {
    --fire_clock;
    return;
  }
  fire_clock = (level >= 3 ? 6u : level >= 1 ? 7u : 9u);
  if (turbo) fire_clock >>= 1;
  if (level >= 3) {
    shot(x - 5, y, 0, S_TWIN);
    shot(x + 5, y, 0, S_TWIN);
  } else {
    shot(x, y, 0, S_TWIN);
  }
  if (level >= 2) {
    shot(x - 8, y + 4, -384, S_LEFT);
    shot(x + 8, y + 4, 384, S_RIGHT);
  }
  if (level >= 4 && (fire_clock & 1)) {
    shot(x - 10, y + 6, -768, S_LEFT);
    shot(x + 10, y + 6, 768, S_RIGHT);
  }
  for (i = 0; i < dw_allies; ++i) {
    uint8_t t = (uint8_t)(trail_i - 10u - i * 10u) & 31u;
    shot((int16_t)dw_trail[t << 1] + 4, (int16_t)dw_trail[(t << 1) + 1], 0,
         dw_profile.ally >= 3 ? S_TWIN : S_SOLO);
  }
  if (!(dw_clock & 7u)) dw_sfx(SFX_SHOT);
}

static void burst(void) {
  uint8_t i, damage;
  DwShot *b = dw_bullets;
  DwFoe *e = dw_foes;
  dw_energy -= 50;
  dw_burst = 40;
  dw_invincible = 60;
  ring_t = 1;
  dw_shake = 14;
  for (i = 0; i < DW_BULLETS; ++i, ++b) if (b->on) {
    b->on = 0;
    if (i & 1u) boom((int16_t)HI(b->x) - BIAS, (int16_t)HI(b->y) - BIAS, F_SPARK);
    dw_points(2);
  }
  damage = 8u + dw_profile.reactor * 4u;
  for (i = 0; i < DW_FOES; ++i, ++e) if (e->hp) {
    if (e->hp > damage) {
      e->hp -= damage;
      e->flash = 6;
    } else {
      kill_foe(e);
      dw_run_tokens += 2;
    }
  }
  if (dw_phase == P_BOSS) {
    damage = 24u + dw_profile.reactor * 8u;
    dw_boss_hp = dw_boss_hp > damage ? dw_boss_hp - damage : 0;
  }
  dw_sfx(SFX_BURST);
}

static void steer(void) {
  uint8_t speed, keys = dw_keys;
  dw_focus = (keys & J_A) != 0;
  speed = dw_focus ? 1 : 2;
  if ((keys & (J_LEFT | J_RIGHT)) && (keys & (J_UP | J_DOWN)))
    speed = dw_focus ? (dw_frame & 1u) : ((dw_frame & 1u) ? 1 : 2);
  if (keys & J_LEFT) dw_px -= speed;
  if (keys & J_RIGHT) dw_px += speed;
  if (keys & J_UP) dw_py -= speed;
  if (keys & J_DOWN) dw_py += speed;
  if (dw_px < 0) dw_px = 0;
  else if (dw_px > 144) dw_px = 144;
  if (dw_py < 8) dw_py = 8;
  else if (dw_py > 110) dw_py = 110;
  if (keys & J_LEFT) bank_dir = -1;
  else if (keys & J_RIGHT) bank_dir = 1;
  else bank_dir = 0;
  if (keys & (J_LEFT | J_RIGHT | J_UP | J_DOWN)) {
    trail_i = (trail_i + 1u) & 31u;
    dw_trail[trail_i << 1] = (uint8_t)dw_px;
    dw_trail[(trail_i << 1) + 1] = (uint8_t)dw_py;
  }
}

/* ---------------------------------------------------------- projectiles */
/* The busiest loops are hand-written SM83: shots and bullets are records of
   {on, x lo, x hi, vx lo, vx hi, y lo, y hi, vy lo, vy hi, kind}. */
uint8_t hit_cx, hit_cy, hit_flag, twin_dmg, hit_lx, hit_ly;
/* Live boxes, packed: half width, biased centre x, half height, biased
   centre y; a zero width ends the list. box_foe maps each to its foe
   (DW_FOES for the boss) and shots add their damage to box_dmg. */
uint8_t boxes[(DW_FOES + 1) * 4 + 1];
uint8_t box_dmg[DW_FOES + 1];
static uint8_t box_foe[DW_FOES + 1], box_n;
/* Foe sprites, prepared with the boxes: OAM x, OAM y, tile, attribute and
   width in 8x16 objects (0 hides it). foe_draw_of maps foe to entry. */
uint8_t foe_draw[DW_FOES * 5];
static uint8_t foe_draw_of[DW_FOES];
const uint8_t *em_ptr;
uint8_t em_count, em_attr;

/* Move every hostile bullet; retire it off screen; flag a hit when one
   comes within two pixels of the player's core (hit_cx, hit_cy). */
static void bullets_tick(void) __naked {
  __asm
    ld hl, #_dw_bullets
    ld b, #DW_BULLETS
  1$:
    ld a, (hl+)
    or a
    jr nz, 2$
    ld de, #9
    add hl, de
    dec b
    jr nz, 1$
    ret
  2$:
    ld a, (hl+)
    ld e, a
    ld a, (hl+)
    ld d, a
    ld a, (hl+)
    add a, e
    ld e, a
    ld a, (hl-)
    adc a, d
    ld d, a
    dec hl
    ld (hl), d
    dec hl
    ld (hl), e
    inc hl
    inc hl
    inc hl
    inc hl
    ld a, (hl+)
    ld e, a
    ld a, (hl+)
    ld c, a
    ld a, (hl+)
    add a, e
    ld e, a
    ld a, (hl-)
    adc a, c
    ld c, a
    dec hl
    ld (hl), c
    dec hl
    ld (hl), e
    ld a, c
    cp #20
    jr c, 7$
    cp #165
    jr nc, 7$
    ld a, d
    cp #22
    jr c, 7$
    cp #203
    jr nc, 7$
    push hl
    ld e, #2
    ld h, #4
    ld a, (_hit_cx)
    ld l, a
    ld a, d
    sub l
    add a, e
    cp h
    jr z, 3$
    jr nc, 5$
  3$:
    ld a, (_hit_cy)
    ld l, a
    ld a, c
    sub l
    add a, e
    cp h
    jr z, 4$
    jr nc, 5$
  4$:
    ld a, #1
    ld (_hit_flag), a
    pop hl
    jr 7$
  5$:
    pop hl
    ld de, #5
    add hl, de
    dec b
    jp nz, 1$
    ret
  7$:
    ld de, #-5
    add hl, de
    xor a
    ld (hl), a
    ld de, #10
    add hl, de
    dec b
    jp nz, 1$
    ret
  __endasm;
}

/* Move every player shot; retire it off screen or on the first box it
   enters, adding its damage (twin_dmg for twin bolts, else 1) to box_dmg. */
static void shots_tick(void) __naked {
  __asm
    ld hl, #_dw_shots
    ld c, #DW_SHOTS
  1$:
    ld a, (hl)
    or a
    jp z, 8$
    push bc
    push hl
    inc hl
    ld a, (hl+)
    ld e, a
    ld a, (hl+)
    ld d, a
    ld a, (hl+)
    add a, e
    ld e, a
    ld a, (hl-)
    adc a, d
    ld d, a
    dec hl
    ld (hl), d
    dec hl
    ld (hl), e
    inc hl
    inc hl
    inc hl
    inc hl
    ld a, (hl+)
    ld e, a
    ld a, (hl+)
    ld b, a
    ld a, (hl+)
    add a, e
    ld e, a
    ld a, (hl-)
    adc a, b
    ld b, a
    dec hl
    ld (hl), b
    dec hl
    ld (hl), e
    ld a, b
    cp #20
    jr c, 6$
    cp #201
    jr nc, 6$
    ld a, d
    cp #24
    jr c, 6$
    cp #201
    jr nc, 6$
    ld hl, #_boxes
    ld c, #0
  2$:
    ld a, (hl+)
    or a
    jr z, 44$
    ld e, a
    ld a, d
    sub (hl)
    add a, e
    sla e
    cp e
    jr z, 3$
    jr nc, 41$
  3$:
    inc hl
    ld a, (hl+)
    ld e, a
    ld a, b
    sub (hl)
    add a, e
    sla e
    cp e
    jr z, 9$
    jr c, 9$
    jr 43$
  41$:
    inc hl
  42$:
    inc hl
  43$:
    inc hl
    inc c
    jr 2$
  44$:
    pop hl
    pop bc
    jr 8$
  9$:
    pop hl
    push hl
    ld de, #9
    add hl, de
    ld a, (hl)
    cp #DW_S_SHOT
    ld a, #1
    jr nz, 10$
    ld a, (_twin_dmg)
  10$:
    ld e, a
    ld hl, #_box_dmg
    ld a, l
    add a, c
    ld l, a
    jr nc, 11$
    inc h
  11$:
    ld a, (hl)
    add a, e
    ld (hl), a
    ld a, d
    ld (_hit_lx), a
    ld a, b
    ld (_hit_ly), a
  6$:
    pop hl
    xor a
    ld (hl), a
    pop bc
  8$:
    ld de, #10
    add hl, de
    dec c
    jp nz, 1$
    ret
  __endasm;
}

/* Emit sprites for em_count records at em_ptr: each record carries its
   tile in the kind byte, and every projectile is centred on (3, 3) of its
   8x16 object, so OAM x = high byte - 27 and y = high byte - 19. */
static void emit_projectiles(void) __naked {
  __asm
    ld a, (_dw_oam_n)
    add a, a
    add a, a
    ld e, a
    ld hl, #_dw_oam
    ld a, (hl+)
    ld h, (hl)
    ld l, a
    ld a, l
    add a, e
    ld e, a
    ld a, h
    adc a, #0
    ld d, a
    ld a, (_em_count)
    ld c, a
    ld hl, #_em_ptr
    ld a, (hl+)
    ld h, (hl)
    ld l, a
  1$:
    ld a, (hl)
    or a
    jr z, 8$
    push hl
    inc hl
    inc hl
    ld a, (hl)
    sub #27
    ld b, a
    dec a
    cp #167
    jr nc, 7$
    ld a, l
    add a, #4
    ld l, a
    jr nc, 2$
    inc h
  2$:
    ld a, (hl)
    sub #19
    ld (de), a
    dec a
    cp #159
    jr nc, 7$
    ld a, (_dw_oam_n)
    cp #40
    jr nc, 77$
    inc a
    ld (_dw_oam_n), a
    inc de
    ld a, b
    ld (de), a
    inc de
    inc hl
    inc hl
    inc hl
    ld a, (hl)
    ld (de), a
    inc de
    ld a, (_em_attr)
    ld (de), a
    inc de
  7$:
    pop hl
  8$:
    ld a, l
    add a, #10
    ld l, a
    jr nc, 81$
    inc h
  81$:
    dec c
    jr nz, 1$
    ret
  77$:
    pop hl
    ret
  __endasm;
}

/* Build this frame's live boxes and foe sprites, and test each foe
   against the player's body. */
static void foe_boxes(void) {
  uint8_t j, y, x, w, wide, attr;
  uint8_t pcx = (uint8_t)(dw_px + 8 + BIAS), pcy = (uint8_t)(dw_py + 8 + BIAS);
  uint8_t live = dw_phase != P_DEAD;
  uint8_t *b = boxes, *d = foe_draw;
  DwFoe *e = dw_foes;
  box_n = 0;
  for (j = 0; j < DW_FOES; ++j, ++e, d += 5) {
    foe_draw_of[j] = 0xFF;
    d[4] = 0;
    if (!e->hp) continue;
    y = HI(e->y);
    x = HI(e->x);
    wide = 2;
    if (e->kind == K_HEAVY) {
      wide = 4;
      d[2] = DW_S_HEAVY + ((e->t >> 3) & 1u) * 8u;
      attr = OP_FOE;
    } else if (e->kind == K_GUNNER) {
      d[2] = DW_S_GUNNER + ((e->t >> 3) & 1u) * 4u;
      attr = OP_FOE2;
    } else {
      d[2] = DW_S_DRONE + ((e->t >> 3) & 1u) * 4u;
      attr = OP_FOE;
    }
    if (e->flash) {
      --e->flash;
      attr = OP_FIRE;
    }
    d[0] = x - 24u;
    d[1] = y - 16u;
    d[3] = attr;
    d[4] = wide;
    foe_draw_of[j] = j;
    if (y < 10u) continue;
    w = wide == 4 ? 16 : 9;
    x += wide == 4 ? 16u : 8u;
    y += 8u;
    b[0] = w;
    b[1] = x;
    b[2] = 9;
    b[3] = y;
    b += 4;
    box_dmg[box_n] = 0;
    box_foe[box_n++] = j;
    /* Body contact: a 10-pixel reach around small foes, 18 for heavies. */
    ++w;
    if (live && (uint8_t)(x - pcx + w) < (uint8_t)(w << 1) && (uint8_t)(y - pcy + 10u) < 20u) {
      if (e->kind != K_HEAVY) {
        kill_foe(e);
        b -= 4;
        --box_n;
      }
      hurt();
    }
  }
  if (dw_phase == P_BOSS) {
    b[0] = 40;
    b[1] = (uint8_t)(bx + 48 + BIAS);
    b[2] = 24;
    b[3] = (uint8_t)(by + 26 + BIAS);
    if (live && (uint8_t)(b[1] - pcx + 36u) < 72u && (uint8_t)(b[3] - pcy + 20u) < 40u) hurt();
    b += 4;
    box_dmg[box_n] = 0;
    box_foe[box_n++] = DW_FOES;
  }
  b[0] = 0;
}

/* Emit the prepared foe sprites: 2 or 4 objects each, tiles two apart. */
static void emit_foes(void) __naked {
  __asm
    ld hl, #_foe_draw
    ld c, #DW_FOES
  1$:
    push hl
    ld de, #4
    add hl, de
    ld a, (hl)
    or a
    jr z, 8$
    ld b, a
    pop hl
    push hl
    ld a, (hl+)
    ld d, a
    ld a, (hl+)
    ld e, a
    dec a
    cp #159
    jr nc, 8$
    ld a, (hl+)
    push af
    ld a, (hl)
    ld h, a
    pop af
    ld l, a
  2$:
    ld a, d
    dec a
    cp #167
    jr nc, 5$
    ld a, (_dw_oam_n)
    cp #40
    jr nc, 8$
    push bc
    push hl
    inc a
    ld (_dw_oam_n), a
    dec a
    add a, a
    add a, a
    ld c, a
    ld b, #0
    ld hl, #_dw_oam
    ld a, (hl+)
    ld h, (hl)
    ld l, a
    add hl, bc
    pop bc
    ld (hl), e
    inc hl
    ld (hl), d
    inc hl
    ld (hl), c
    inc hl
    ld (hl), b
    ld h, b
    ld l, c
    pop bc
  5$:
    inc l
    inc l
    ld a, d
    add a, #8
    ld d, a
    dec b
    jr nz, 2$
  8$:
    pop hl
    ld de, #5
    add hl, de
    dec c
    jr nz, 1$
    ret
  __endasm;
}

static void move_shots(void) {
  uint8_t k, j, d;
  DwFoe *e;
  foe_boxes();
  twin_dmg = 2u + (dw_profile.weapon >= 3);
  shots_tick();
  for (k = 0; k < box_n; ++k) {
    d = box_dmg[k];
    if (!d) continue;
    j = box_foe[k];
    if (j == DW_FOES) {
      dw_boss_hp = dw_boss_hp > d ? dw_boss_hp - d : 0;
      if (!flash_t) flash_t = 4;
      if (!(dw_clock & 3u)) boom((int16_t)hit_lx - BIAS, (int16_t)hit_ly - BIAS, F_SPARK);
      continue;
    }
    e = &dw_foes[j];
    if (!e->hp) continue;
    if (e->hp > d) {
      e->hp -= d;
      e->flash = 3;
      foe_draw[j * 5u + 3] = OP_FIRE;
      dw_sfx(SFX_HIT);
    } else {
      kill_foe(e);
    }
  }
}

static void move_bullets(void) {
  hit_cx = (uint8_t)(dw_px + 8 + BIAS);
  hit_cy = (uint8_t)(dw_py + 7 + BIAS);
  hit_flag = 0;
  bullets_tick();
  if (hit_flag) hurt();
}

static void collect(DwDrop *d) {
  d->on = 0;
  switch (d->kind) {
    case D_COIN:
      dw_run_tokens += 1u + (dw_burst ? 1u : 0u);
      dw_points(25);
      dw_sfx(SFX_COIN);
      break;
    case D_POWER:
      if (dw_power < 2) ++dw_power;
      else dw_points(500);
      dw_sfx(SFX_POWER);
      break;
    case D_REPAIR:
      if (dw_hp < 3u + dw_profile.shield) ++dw_hp;
      else dw_points(500);
      dw_sfx(SFX_POWER);
      break;
    case D_BOLT:
      turbo = 480;
      dw_energy = 100;
      dw_sfx(SFX_POWER);
      break;
    default:
      if (run_allies < 2) ++run_allies;
      dw_sfx(SFX_POWER);
      break;
  }
  dw_allies = dw_profile.ally + run_allies;
  if (dw_profile.ally >= 3 || dw_allies > 2) dw_allies = 2;
}

static void move_drops(void) {
  uint8_t i, x, y, reach = 24u + dw_profile.reactor * 10u, alive = dw_phase != P_DEAD;
  uint8_t cx = (uint8_t)(dw_px + 8 + BIAS), cy = (uint8_t)(dw_py + 8 + BIAS);
  DwDrop *d = dw_drops;
  for (i = 0; i < DW_DROPS; ++i, ++d) {
    if (!d->on) continue;
    ++d->t;
    x = HI(d->x);
    y = HI(d->y);
    if (alive && (uint8_t)(x - cx + reach) < (uint8_t)(reach << 1) && (uint8_t)(y - cy + reach) < (uint8_t)(reach << 1)) {
      d->x += x < cx ? 384 : -384;
      d->y += y < cy ? 384 : -384;
    } else {
      d->y += 144;
      d->x += sin64[(d->t << 2) & 63];
    }
    if (y > BIAS + 136) {
      d->on = 0;
      continue;
    }
    if (alive && (uint8_t)(x - cx + 9u) < 18u && (uint8_t)(y - cy + 9u) < 18u) collect(d);
  }
}

/* ------------------------------------------------------------------ HUD */
static uint8_t hud_dirty;

static void hud_tile(uint8_t x, uint8_t y, uint8_t t, uint8_t pal) {
  uint8_t i = (uint8_t)(y << 5) + x;
  dw_hudbuf[i] = t;
  dw_hudatt[i] = pal;
  hud_dirty = 1;
}

static const uint16_t pow10[5] = {10000, 1000, 100, 10, 1};

/* Decimal digits by repeated subtraction: no division on this CPU. Row 1
   pads with spaces; row 0 (the bordered score) keeps leading zeros. */
static void hud_digits(uint8_t x, uint8_t y, uint16_t n, uint8_t width, uint8_t base) {
  uint8_t k = 5u - width, d, lead = y;
  uint16_t p10;
  for (; k < 5; ++k, ++x) {
    p10 = pow10[k];
    d = 0;
    while (n >= p10) {
      n -= p10;
      ++d;
    }
    if (d || k == 4) lead = 0;
    hud_tile(x, y, lead ? ' ' : base + d, PAL_HUD);
  }
}

static void hud_flush(void) {
  uint16_t dst;
  if (!hud_dirty) return;
  dst = 0x9C00u + ((uint16_t)hud_row << 5);
  dw_xfer(dw_hudbuf, dst, 4, 0);
  dw_xfer(dw_hudatt, dst, 4, 1);
  hud_dirty = 0;
}

static void hud_full(void) {
  uint8_t i;
  for (i = 0; i < 20; ++i) {
    hud_tile(i, 0, DW_T_EDGE, PAL_HUD);
    hud_tile(i, 1, DW_T_BLANK, PAL_HUD);
  }
  hud_tile(0, 1, DW_T_BOLT, PAL_HUD);
  hud_tile(7, 1, DW_T_COIN, PAL_HUD);
  hud_score = hud_tokens = 0xFFFF;
  hud_hp = hud_hpmax = hud_energy = hud_chain = hud_power = hud_boss = hud_mode = 0xFF;
  WX_REG = 7;
  WY_REG = hud_row ? 64 : 128;
  SHOW_WIN;
}

/* One HUD element per frame keeps the cost of any single frame low. */
static uint8_t hud_turn, hud_hold;

static void hud(void) {
  uint8_t i, v, max = 3u + dw_profile.shield, turn = ++hud_turn & 3u;
  if (hud_hp != dw_hp || hud_hpmax != max) {
    for (i = 0; i < 6; ++i)
      hud_tile(i, 0, i < dw_hp ? DW_T_HEART : i < max ? DW_T_HEART_OFF : DW_T_EDGE, PAL_HUDBAR);
    hud_hp = dw_hp;
    hud_hpmax = max;
  }
  if (turn == 0 && hud_score != dw_score) {
    hud_digits(14, 0, dw_score, 5, DW_T_D0);
    hud_score = dw_score;
  }
  v = chain >= 4 ? 1u + (chain >> 2) : 0;
  if (v > 8) v = 8;
  if (hud_chain != v) {
    hud_tile(10, 0, v ? DW_T_CHAIN : DW_T_EDGE, PAL_HUD);
    hud_tile(11, 0, v ? DW_T_D0 + v : DW_T_EDGE, PAL_HUD);
    hud_chain = v;
  }
  v = dw_energy >> 1;     /* 0..50 pixels over five tiles */
  if (v > 40) v = 40;
  if (turn == 1 && hud_energy != v) {
    for (i = 0; i < 5; ++i) {
      uint8_t fill = v > i * 8u ? v - i * 8u : 0;
      hud_tile(1 + i, 1, DW_T_BAR0 + (fill > 8 ? 8 : fill), PAL_HUDBAR);
    }
    hud_energy = v;
  }
  if (turn == 2 && hud_tokens != dw_run_tokens) {
    hud_digits(8, 1, dw_run_tokens > 999 ? 999 : dw_run_tokens, 3, '0');
    hud_tokens = dw_run_tokens;
  }
  if (dw_phase >= P_BOSS_IN && dw_phase <= P_BOSS) {
    if (hud_mode != 1) {
      hud_tile(12, 1, DW_T_BOSS, PAL_HUDBAR);
      hud_mode = 1;
      hud_boss = 0xFF;
    }
    v = hud_boss;
    if (!(dw_frame & 3u) || hud_boss == 0xFF)
      v = (uint8_t)((dw_boss_hp * 28u + dw_boss_max - 1u) / dw_boss_max);
    if (turn == 3 && hud_boss != v) {
      for (i = 0; i < 7; ++i) {
        uint8_t fill = v > i * 4u ? v - i * 4u : 0;
        hud_tile(13 + i, 1, DW_T_BBAR0 + (fill > 4 ? 4 : fill), PAL_HUDBAR);
      }
      hud_boss = v;
    }
  } else {
    if (hud_mode != 0) {
      hud_tile(12, 1, DW_T_POWER, PAL_HUD);
      for (i = 17; i < 20; ++i) hud_tile(i, 1, DW_T_BLANK, PAL_HUD);
      hud_mode = 0;
      hud_power = 0xFF;
    }
    v = fire_level();
    if (hud_power != v) {
      for (i = 0; i < 4; ++i) hud_tile(13 + i, 1, i < v ? '#' : '&', PAL_HUD);
      hud_power = v;
    }
  }
  if (!hud_hold) hud_flush();
}

/* Draw every HUD element now (after hud_full), queueing a single copy. */
static void hud_all(void) {
  hud_hold = 1;
  hud();
  hud();
  hud();
  hud();
  hud_hold = 0;
  hud_flush();
}

/* ---------------------------------------------------------------- bosses */
/* Ease the boss's palettes towards the arena colour as it burns out. */
static void boss_fade(void) {
  uint8_t i;
  uint16_t base = dw_pal[0], c;
  uint8_t br = base & 31, bg = (base >> 5) & 31, bb = (base >> 10) & 31, r, g, b;
  for (i = 4; i < 24; ++i) {
    c = dw_pal[i];
    r = c & 31;
    g = (c >> 5) & 31;
    b = (c >> 10) & 31;
    r = r > br ? r - ((r - br + 3) >> 2) : r + ((br - r + 3) >> 2);
    g = g > bg ? g - ((g - bg + 3) >> 2) : g + ((bg - g + 3) >> 2);
    b = b > bb ? b - ((b - bb + 3) >> 2) : b + ((bb - b + 3) >> 2);
    dw_pal[i] = r | ((uint16_t)g << 5) | ((uint16_t)b << 10);
  }
  dw_pal_commit_part(4, 20);
}

static void sector_complete(void) {
  dw_points(1000u + (uint16_t)dw_sector * 250u);
  dw_run_tokens += 10u * dw_sector;
  if (dw_profile.cleared < dw_sector) dw_profile.cleared = dw_sector;
  dw_bank(0);
  if (dw_sector == 4) {
    dw_finish(1);
  } else {
    dw_song(SONG_HANGAR);
    dw_fade_to(0);
    HIDE_WIN;
    dw_state = DW_SHOP;
    dw_menu_row = 0;
    dw_screen();
  }
}

static void run_phase(void) {
  uint8_t i;
  ++dw_phase_t;
  switch (dw_phase) {
    case P_INTRO:
      if (dw_phase_t >= 120) {
        dw_phase = P_WAVES;
        dw_state = DW_FLIGHT;
        dw_phase_t = 0;
      }
      break;
    case P_WAVES:
      dw_waves((uint8_t)(dw_cam >> 4));
      if (dw_cam >= DW_LEVEL_ROWS * 8u) {
        uint8_t alive = 0;
        for (i = 0; i < DW_FOES; ++i) alive |= dw_foes[i].hp;
        if (!alive) {
          dw_phase = P_WARN;
          dw_phase_t = 0;
          dw_song(SONG_BOSS);
          dw_sfx(SFX_WARN);
          sky_text(6, 6, "WARNING");
          sky_text(3, 8, lab_names[SECTOR_IDX]);
        }
      }
      break;
    case P_WARN:
      if ((dw_phase_t & 7u) == 0) {
        dw_pal[4 * PAL_TEXT + 3] = (dw_phase_t & 8u) ? 0x7FFF : 0x14BF;
        dw_pal_commit_part(4 * PAL_TEXT, 4);
        if (dw_phase_t < 90 && !(dw_phase_t & 31u)) dw_sfx(SFX_WARN);
      }
      if (dw_phase_t >= 100 && dw_phase_t < 109) {
        /* Flatten map rows 7-31 (all off-screen or already flat). */
        dw_arena_rows(7u + (uint8_t)(dw_phase_t - 100u) * 3u, dw_phase_t == 108 ? 1 : 3, dw_sector);
      } else if (dw_phase_t == 110) {
        /* Switch to boss scrolling: rows 0-6 sit just above the screen. */
        bx = 0;
        by = -56;
        dw_scx = 0;
        dw_scy = 56;
      } else if (dw_phase_t >= 112 && dw_phase_t < 115) {
        dw_arena_rows((uint8_t)(dw_phase_t - 112u) * 3u, dw_phase_t == 114 ? 1 : 3, dw_sector);
      } else if (dw_phase_t == 121) {
        dw_pal_boss(dw_sector);
        dw_pal_commit_part(4, 20);
        dw_boss_begin();
        dw_phase = P_BOSS_IN;
        dw_phase_t = 0;
      }
      break;
    case P_BOSS_IN:
      ++by;
      if (by >= 8) {
        dw_phase = P_BOSS;
        dw_phase_t = 0;
      }
      break;
    case P_BOSS:
      dw_boss_tick();
      if (!dw_boss_hp) {
        dw_phase = P_BOSS_DIE;
        dw_phase_t = 0;
        for (i = 0; i < DW_BULLETS; ++i) dw_bullets[i].on = 0;
        dw_sfx(SFX_BIGBOOM);
        dw_song(SONG_NONE);
      }
      break;
    case P_BOSS_DIE:
      if (!(dw_phase_t % 5u)) {
        boom(bx + 12 + (int16_t)(dw_random() % 72u), by + 4 + (int16_t)(dw_random() % 44u), F_BOOM);
        dw_shake = 6;
        if (!(dw_phase_t % 15u)) dw_sfx(SFX_BOOM);
      }
      if (dw_phase_t >= 80 && !(dw_phase_t % 6u)) boss_fade();
      if (dw_phase_t >= 146 && dw_phase_t < 149) {
        dw_arena_rows((uint8_t)(dw_phase_t - 146u) * 3u, 3, 0);
      }
      if (dw_phase_t == 150) {
        dw_world_palettes();
        dw_pal_commit_part(0, 24);
        for (i = 0; i < 6; ++i) drop(bx + 24 + i * 9, by + 20 + (i & 1u) * 8, D_COIN);
        dw_song(SONG_CLEAR);
        dw_sfx(SFX_BIGBOOM);
        dw_phase = P_CLEAR;
        dw_phase_t = 0;
        sky_text(4, 9, "SECTOR CLEAR");
      }
      break;
    case P_CLEAR:
      if (dw_phase_t == 40 || dw_phase_t == 70) for (i = 0; i < 3; ++i) drop(bx + 30 + i * 16, by + 10, D_COIN);
      if (dw_phase_t >= 230) sector_complete();
      break;
    case P_DEAD:
      ++death_t;
      if (death_t < 60 && !(death_t % 6u)) {
        boom(dw_px + (int16_t)(dw_random() & 15u), dw_py + (int16_t)(dw_random() & 15u), F_BOOM);
        dw_shake = 8;
      }
      if (death_t == 100) {
        dw_bank(0);
        HIDE_WIN;
        dw_finish(0);
      }
      break;
  }
}

/* -------------------------------------------------------------- drawing */
/* Tiles, attributes and biased-centre to OAM offsets, by kind. */
static const uint8_t boom_frame[18] = {
  DW_S_BOOM5, DW_S_BOOM5, DW_S_BOOM5, DW_S_BOOM4, DW_S_BOOM4, DW_S_BOOM4,
  DW_S_BOOM3, DW_S_BOOM3, DW_S_BOOM3, DW_S_BOOM2, DW_S_BOOM2, DW_S_BOOM2,
  DW_S_BOOM1, DW_S_BOOM1, DW_S_BOOM1, DW_S_BOOM, DW_S_BOOM, DW_S_BOOM};
static void draw(void) {
  uint8_t i, tile, ox, oy;
  DwDrop *d;
  DwFx *f;
  static const uint8_t drop_tile[5] = {DW_S_COIN, DW_S_POWER, DW_S_REPAIR, DW_S_BOLT, DW_S_ALLY_PICK};
  static const uint8_t drop_pal[5] = {OP_GOLD, OP_GOLD, OP_SHADOW, OP_ENERGY, OP_PLANE};

  /* The player first, so the plane always wins the scanline budget. */
  ox = (uint8_t)(dw_px + 8);
  oy = oy_of(dw_py);
  if (dw_phase != P_DEAD && (!dw_invincible || (dw_invincible & 4u) || dw_burst)) {
    if (dw_focus) spr(ox + 6u, oy + 5u, DW_S_CORE, OP_ENERGY);
    if (bank_dir < 0) spr16(ox, oy, DW_S_PLANE_BANK, OP_PLANE);
    else if (bank_dir > 0) spr16(ox, oy, DW_S_PLANE_BANK, OP_PLANE | S_FLIPX);
    else spr16(ox, oy, DW_S_PLANE, OP_PLANE);
  }
  /* Hostile bullets next: they must never vanish. */
  if (clip_oy == 160) {
    em_ptr = (const uint8_t *)dw_bullets;
    em_count = DW_BULLETS;
    em_attr = OP_BULLET;
    emit_projectiles();
  }
  if (dw_phase != P_DEAD) {
    for (i = 0; i < dw_allies; ++i) {
      uint8_t t = (uint8_t)(trail_i - 10u - i * 10u) & 31u;
      spr(dw_trail[t << 1] + 12u, dw_trail[(t << 1) + 1] + 22u, (dw_clock & 4u) ? DW_S_ALLY1 : DW_S_ALLY, OP_PLANE);
    }
    if (!(dw_clock & 1u) || dw_focus)
      spr(ox + 4u, oy + 14u, (dw_clock & 2u) ? DW_S_FLAME1 : DW_S_FLAME, OP_FIRE);
  }
  if (dw_phase == P_BOSS || dw_phase == P_BOSS_IN) {
    tile = DW_S_BOSSCORE + ((dw_boss_t >> 3) & 1u) * 4u;
    spr16(ox_of(bx + core_x[SECTOR_IDX]), oy_of(by + core_y[SECTOR_IDX]), tile, flash_t ? OP_FIRE : OP_BULLET);
  }
  emit_foes();
  for (i = 0, d = dw_drops; i < DW_DROPS; ++i, ++d) {
    if (!d->on) continue;
    tile = drop_tile[d->kind];
    if (d->kind == D_COIN) tile += ((d->t >> 2) & 3u) * 2u;
    spr(HI(d->x) - 28u, HI(d->y) - 20u, tile, drop_pal[d->kind]);
  }
  if (clip_oy == 160) {
    em_ptr = (const uint8_t *)dw_shots;
    em_count = DW_SHOTS;
    em_attr = OP_ENERGY;
    emit_projectiles();
  }
  for (i = 0, f = dw_fx; i < DW_FXS; ++i, ++f) {
    if (!f->t) continue;
    --f->t;
    if (f->kind == F_BOOM) {
      tile = boom_frame[f->t];
      spr16(f->x - 32u, f->y - 24u, tile, OP_FIRE);
    } else {
      spr(f->x - 28u, f->y - 19u, (f->t & 4u) ? DW_S_SPARK : DW_S_SPARK1, OP_FIRE);
    }
  }
  if (ring_t) {
    /* The token burst: a shockwave ring racing outwards. */
    uint8_t r = ring_t * 3u;
    int16_t x = dw_px + 4, y = dw_py + 4;
    for (i = 0; i < 8; ++i)
      spr(ox_of(x + ((dir_x[i * 4u] * r) >> 6)), oy_of(y + ((dir_y[i * 4u] * r) >> 6)), DW_S_RING, OP_ENERGY);
    if (++ring_t > 26) ring_t = 0;
  }
  if (dw_phase >= P_BOSS_IN && dw_phase <= P_CLEAR) {
    /* Speed lines keep the sky rushing past while the arena holds still. */
    for (i = 0; i < 4; ++i) {
      uint8_t t = (uint8_t)(dw_clock * 5u + i * 67u);
      spr((uint8_t)(i * 41u + 19u + ((dw_clock >> 6) & 7u) * 3u), t, DW_S_SPEED, OP_ENERGY);
    }
  }
}

/* -------------------------------------------------------- flow control */
void dw_start(void) BANKED {
  uint8_t i, next = dw_state == DW_SHOP;
  if (next) {
    ++dw_sector;
    if (dw_hp < 3u + dw_profile.shield) ++dw_hp;
  } else {
    dw_sector = 1;
    dw_score = dw_kills = dw_run_tokens = 0;
    dw_hp = 3u + dw_profile.shield;
    dw_energy = 60;
    dw_power = dw_victory = 0;
    run_allies = 0;
    dw_seed = dw_clock ^ 0xD071u;
    if (!dw_seed) dw_seed = 1;
    dw_bank(1);
    dw_new_best = 0;
  }
  dw_fade_to(0);
  dw_allies = dw_profile.ally + run_allies;
  if (dw_profile.ally >= 3 || dw_allies > 2) dw_allies = 2;
  chain = chain_t = 0;
  dw_frame = dw_stage_frame = 0;
  dw_boss_hp = dw_boss_max = dw_burst = 0;
  dw_invincible = 0;
  dw_px = 72;
  dw_py = 140;
  dw_focus = 0;
  bank_dir = 0;
  turbo = 0;
  fire_clock = 30;
  ring_t = flash_t = 0;
  dw_shake = 0;
  dw_cam = 0;
  cam_x = 0;
  dw_rivals_begin();
  dw_phase = P_INTRO;
  dw_phase_t = 0;
  for (i = 0; i < DW_FOES; ++i) dw_foes[i].hp = 0;
  for (i = 0; i < DW_SHOTS; ++i) dw_shots[i].on = 0;
  for (i = 0; i < DW_BULLETS; ++i) dw_bullets[i].on = 0;
  for (i = 0; i < DW_DROPS; ++i) dw_drops[i].on = 0;
  for (i = 0; i < DW_FXS; ++i) dw_fx[i].t = 0;
  for (i = 0; i < 32; ++i) {
    dw_trail[i << 1] = 72;
    dw_trail[(i << 1) + 1] = 120;
  }
  trail_i = 0;
  dw_state = DW_TAKEOFF;
  dw_dim = 0;
  /* Build the sector with the palettes black, then fade it in. */
  dw_pal_flight(dw_sector);
  dw_load_fleet(SECTOR_IDX);
  dw_world_begin(dw_sector);
  dw_boss_tiles(dw_sector);
  dw_scx = 0;
  dw_scy = 112;
  for (next_row = 0; next_row < 20; ++next_row) dw_world_row(next_row, 1);
  dw_clear_win();
  hud_row = 0;
  hud_full();
  hud_all();
  {
    char label[9];
    memcpy(label, "SECTOR 1", 9);
    label[7] = '0' + dw_sector;
    sky_text(6, 5, label);
  }
  sky_text((20u - (uint8_t)strlen(sector_names[SECTOR_IDX])) >> 1, 7, sector_names[SECTOR_IDX]);
  sky_text((20u - (uint8_t)strlen(lab_names[SECTOR_IDX])) >> 1, 8, lab_names[SECTOR_IDX]);
  dw_oam_begin();
  draw();
  dw_oam_flip();
  dw_song(dw_sector > 2 ? SONG_FLIGHT_B : SONG_FLIGHT_A);
  dw_sfx(SFX_LAUNCH);
  SCX_REG = dw_scx;
  SCY_REG = dw_scy;
  LCDC_REG |= LCDCF_ON;
  dw_fade_to(8);
}

static void pause_panel(void) {
  uint8_t y;
  VBK_REG = 1;
  fill_win_rect(0, 0, 20, 8, PAL_HUD);
  VBK_REG = 0;
  fill_win_rect(0, 0, 20, 8, DW_T_BLANK);
  for (y = 0; y < 20; ++y) set_win_tile_xy(y, 0, DW_T_EDGE);
  set_win_tiles(7, 1, 6, 1, (const uint8_t *)"PAUSED");
  set_win_tiles(2, 3, 15, 1, (const uint8_t *)"START   RESUME ");
  set_win_tiles(2, 4, 15, 1, (const uint8_t *)"B       END RUN");
  set_win_tiles(2, 5, 17, 1, dw_muted ? (const uint8_t *)"SELECT  SOUND ON " : (const uint8_t *)"SELECT  SOUND OFF");
  set_win_tiles(2, 6, (uint8_t)strlen(sector_names[SECTOR_IDX]), 1, (const uint8_t *)sector_names[SECTOR_IDX]);
}

void dw_resume(void) BANKED {
  dw_state = dw_phase == P_INTRO ? DW_TAKEOFF : DW_FLIGHT;
  dw_dim = 0;
  dw_pal_commit();
  hud_row = 0;
  dw_clear_win();
  hud_full();
  hud_all();
}

static void pause_update(void) {
  if (dw_pressed & J_START) {
    dw_resume();
  } else if (dw_pressed & J_B) {
    dw_bank(0);
    HIDE_WIN;
    dw_finish(0);
  } else if (dw_pressed & J_SELECT) {
    pause_panel();
  }
}

void dw_update(void) BANKED {
  if (dw_state == DW_PAUSE) {
    pause_update();
    if (dw_state == DW_PAUSE) {
      clip_oy = 64;
      draw();
      clip_oy = 160;
    }
    return;
  }
  /* VBlank work first: new scenery rows, then the HUD. */
  if (dw_phase <= P_WAVES) {
    uint16_t want = ((dw_cam + 144u) >> 3) + 2u;
    if (next_row < want) dw_world_row(next_row++, 0);
  }
  hud();
  if ((dw_pressed & J_START) && dw_phase != P_DEAD && dw_phase != P_BOSS_DIE && dw_phase != P_CLEAR) {
    dw_state = DW_PAUSE;
    dw_dim = 1;
    dw_pal_commit();
    hud_row = 8;
    pause_panel();
    hud_full();
    hud_all();
    clip_oy = 64;
    draw();
    clip_oy = 160;
    return;
  }
  ++dw_frame;
  ++dw_stage_frame;
  if (dw_phase <= P_WAVES && (dw_frame & 1u)) ++dw_cam;
  dw_scy = dw_phase < P_BOSS_IN ? (uint8_t)(112u - (uint8_t)dw_cam) : (uint8_t)(0 - by);
  if (dw_phase < P_BOSS_IN) {
    int8_t want = (int8_t)((dw_px - 72) / 5);
    if (cam_x < want) ++cam_x;
    else if (cam_x > want) --cam_x;
    dw_scx = (uint8_t)cam_x;
  } else {
    dw_scx = (uint8_t)(0 - bx);
  }
  if (dw_phase == P_INTRO) {
    if (dw_py > 100) dw_py -= 1;
    if (dw_phase_t > 60) steer();
  } else if (dw_phase != P_DEAD) {
    steer();
  }
  if (dw_invincible) --dw_invincible;
  if (dw_burst) --dw_burst;
  if (turbo) --turbo;
  if (chain_t && !--chain_t) chain = 0;
  if (flash_t) {
    if (flash_t == 4) {
      dw_flash_mask = 0x3E;
      dw_flash_level = 3;
      dw_pal_commit_part(4, 20);
    } else if (flash_t == 2) {
      dw_flash_mask = 0;
      dw_pal_commit_part(4, 20);
    }
    --flash_t;
  }
  if (dw_energy < 100 && !(dw_frame & 7u)) {
    dw_energy += 1u + dw_profile.reactor;
    if (dw_energy > 100) dw_energy = 100;
  }
  if (dw_phase != P_DEAD && dw_phase != P_INTRO) {
    if ((dw_pressed & J_B) && dw_energy >= 50) burst();
    if (dw_phase != P_CLEAR && dw_phase != P_BOSS_DIE) fire();
  }
  run_phase();
  if (dw_state != DW_FLIGHT && dw_state != DW_TAKEOFF) return;
  dw_foes_tick(dw_phase == P_WAVES);
  move_shots();
  move_bullets();
  move_drops();
  draw();
}
