#include "runtime.h"
#include "digits.h"
#include <string.h>
const char game_title[] = "PRISM WELL", game_tagline[] = "LINE UP   THE LIGHT",
           game_controls1[] = "LEFT RIGHT MOVE",
           game_controls2[] = "A B CYCLE  UP DROP",
           game_goal[] = "MATCH 3 IN ANY LINE";
#define C RGB
const uint16_t game_palette[32] = {
    C(2, 2, 6),    C(8, 8, 14),   C(18, 18, 24), C(31, 31, 30), C(2, 2, 6),
    C(14, 2, 6),   C(28, 6, 10),  C(31, 20, 22), C(2, 2, 6),    C(16, 9, 2),
    C(30, 20, 4),  C(31, 30, 16), C(2, 2, 6),    C(2, 12, 6),   C(6, 24, 12),
    C(20, 31, 20), C(2, 2, 6),    C(6, 6, 12),   C(11, 11, 19), C(18, 18, 26),
    C(2, 2, 6),    C(3, 6, 18),   C(7, 14, 30),  C(20, 26, 31), C(2, 2, 6),
    C(10, 4, 16),  C(20, 9, 28),  C(29, 22, 31), C(2, 2, 6),    C(8, 8, 8),
    C(14, 13, 12), C(21, 20, 18)};
#define W 7
#define H 15
#define CELLS 105
#define STONE 6
#define PRISM 7
#define GEM_TILE 75
#define STONE_TILE 81
#define PRISM_TILE 82
#define FLASH_TILE 83
#define GHOST_TILE 84
#define PLAY 0
#define FLASH 1
#define SETTLE 2
#define CLEAR 3
#define LIGHT_GOAL 30
uint8_t board[CELLS], marks[CELLS];
uint8_t piece[3], next_piece[3];
int8_t piece_y;
uint8_t piece_x, fall_wait, lock_wait, hold, well, light, stones, chain,
    best_chain, state, state_wait, prism_in, colors_n, dirty, banner_t;
/* busy marks an update that already scanned or shattered the well; the
   redraw then waits a frame so the update stays inside one frame. */
uint8_t redraw, busy, shown_x;
int8_t shown_ghost;
uint16_t cleared, shown_score;
const char *banner;
/* Palette and tile for each cell value: empty, five gems, stone, prism. */
const uint8_t cell_pal[8] = {0, 1, 2, 3, 5, 6, 7, 0};
const uint8_t cell_tile[8] = {0, 76, 77, 78, 79, 80, STONE_TILE, PRISM_TILE};
const uint8_t fall_speed[6] = {0, 18, 16, 14, 12, 10};
/* Stone rows per well, bottom row first; bit n is column n. */
const uint8_t stone_rows[6][3] = {{0, 0, 0},       {0, 0, 0},
                                  {0x55, 0, 0},    {0x6B, 0x22, 0},
                                  {0x77, 0x55, 0}, {0x77, 0x63, 0x22}};
/* A soft triangle for the wave channel, and a pentatonic chain scale. */
const uint8_t wave[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                          0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};
const uint16_t scale[7] = {1923, 1936, 1949, 1964, 1974, 1985, 1992};
const int8_t step_x[4] = {1, 0, 1, 1}, step_y[4] = {0, 1, 1, -1};
/* Index offset of each scan direction, and the first cell of each row. */
const int8_t step_n[4] = {1, W, W + 1, 1 - W};
const uint8_t row7[H] = {0, 7, 14, 21, 28, 35, 42, 49, 56, 63, 70, 77, 84, 91, 98};
static void chime(uint8_t n) {
  uint16_t f = scale[n > 6 ? 6 : n];
  NR30_REG = 0x80;
  NR31_REG = 0xC0;
  NR32_REG = 0x20;
  NR33_REG = (uint8_t)f;
  NR34_REG = 0xC0 | (uint8_t)(f >> 8);
}
static uint8_t fits(uint8_t x, int8_t y) {
  uint8_t i;
  int8_t r;
  for (i = 0; i < 3; i++) {
    r = y + i;
    if (r >= H)
      return 0;
    if (r >= 0 && board[row7[r] + x])
      return 0;
  }
  return 1;
}
static void deal(void) {
  uint8_t i;
  if (!--prism_in) {
    prism_in = 20;
    next_piece[0] = next_piece[1] = next_piece[2] = PRISM;
    return;
  }
  for (i = 0; i < 3; i++)
    next_piece[i] = 1 + random16() % colors_n;
}
static void spawn(void) {
  memcpy(piece, next_piece, 3);
  deal();
  redraw = 1;
  piece_x = 3;
  piece_y = -2;
  fall_wait = lock_wait = 0;
  state = PLAY;
  if (board[3])
    ended = 1;
}
static void setup_well(void) {
  uint8_t r, c, bits;
  memset(board, 0, sizeof(board));
  memset(marks, 0, sizeof(marks));
  stones = light = chain = 0;
  colors_n = well >= 3 ? 5 : 4;
  for (r = 0; r < 3; r++) {
    bits = stone_rows[well][r];
    for (c = 0; c < W; c++)
      if (bits & (1 << c)) {
        board[row7[H - 1 - r] + c] = STONE;
        stones++;
      }
  }
  banner = well == 1 ? "SHATTER 30 LIGHTS" : "BREAK EVERY STONE";
  banner_t = 60;
  dirty = 1;
  deal();
  spawn();
}
void game_reset(void) {
  uint8_t i;
  health = 0;
  well = 1;
  cleared = 0;
  best_chain = 0;
  prism_in = 20;
  hold = 0;
  NR30_REG = 0;
  for (i = 0; i < 16; i++)
    AUD3WAVE[i] = wave[i];
  NR30_REG = 0x80;
  colors_n = 4;
  deal();
  setup_well();
}
/* Marks every run of three or more in the four scan directions. Runs are
   walked only from their first cell, with indices stepped, not multiplied. */
static uint8_t find_matches(void) {
  uint8_t r, c, d, len, k, v, n = 0, m, found = 0;
  int8_t rr, cc, dx, dy;
  busy = 1;
  for (r = 0; r < H; r++)
    for (c = 0; c < W; c++, n++) {
      v = board[n];
      if (!v || v >= STONE)
        continue;
      for (d = 0; d < 4; d++) {
        dx = step_x[d];
        dy = step_y[d];
        rr = r - dy;
        cc = c - dx;
        if (rr >= 0 && rr < H && cc >= 0 && board[n - step_n[d]] == v)
          continue;
        len = 1;
        m = n;
        rr = r + dy;
        cc = c + dx;
        while (rr >= 0 && rr < H && cc < W && board[m + step_n[d]] == v) {
          m += step_n[d];
          len++;
          rr += dy;
          cc += dx;
        }
        if (len >= 3) {
          m = n;
          for (k = 0; k < len; k++, m += step_n[d])
            marks[m] = 1;
          found = 1;
        }
      }
    }
  return found;
}
static void shatter(void) {
  uint8_t n, gems = 0, broken = 0, r, c;
  busy = 1;
  for (n = 0; n < CELLS; n++)
    if (marks[n]) {
      board[n] = 0;
      gems++;
    }
  for (n = 0; n < CELLS; n++) {
    if (!marks[n])
      continue;
    r = n / W;
    c = n % W;
    if (r && board[n - W] == STONE)
      board[n - W] = 0, broken++;
    if (r < H - 1 && board[n + W] == STONE)
      board[n + W] = 0, broken++;
    if (c && board[n - 1] == STONE)
      board[n - 1] = 0, broken++;
    if (c < W - 1 && board[n + 1] == STONE)
      board[n + 1] = 0, broken++;
  }
  memset(marks, 0, sizeof(marks));
  redraw = 1;
  points((uint16_t)gems * 10 * chain * well);
  points((uint16_t)broken * 50 * well);
  light = light + gems > 250 ? 250 : light + gems;
  cleared += gems;
  stones -= broken;
  chime(chain - 1);
  tone(1760 + chain * 30, 0x93);
  if (broken)
    noise(0x46);
  if (chain > best_chain)
    best_chain = chain;
}
/* Drops every unsupported cell one row; returns nonzero while moving. */
static uint8_t settle(void) {
  uint8_t moved = 0, n = CELLS - W;
  while (n--)
    if (board[n] && !board[n + W]) {
      board[n + W] = board[n];
      board[n] = 0;
      moved = redraw = 1;
    }
  return moved;
}
static void lock(void) {
  uint8_t i, below;
  if (piece_y < 0) {
    ended = 1;
    return;
  }
  for (i = 0; i < 3; i++)
    board[row7[piece_y + i] + piece_x] = piece[i];
  redraw = 1;
  noise(0x25);
  chain = 0;
  if (piece[0] == PRISM) {
    below = piece_y + 3 < H ? board[row7[piece_y + 3] + piece_x] : 0;
    for (i = 0; i < 3; i++)
      marks[row7[piece_y + i] + piece_x] = 1;
    if (below && below < STONE)
      for (i = 0; i < CELLS; i++)
        if (board[i] == below)
          marks[i] = 1;
    if (!below)
      points(300);
    chain = 1;
    banner = "PRISM FLARE!";
    banner_t = 40;
    state = FLASH;
    state_wait = 14;
    tone(1990, 0xB6);
    return;
  }
  state = SETTLE;
}
static void control(void) {
  uint8_t t, interval = (keys & J_DOWN) ? 1 : fall_speed[well];
  if (keys & (J_LEFT | J_RIGHT))
    hold++;
  else
    hold = 0;
  if (((pressed & J_LEFT) || ((keys & J_LEFT) && hold > 6 && (hold & 1))) &&
      piece_x && fits(piece_x - 1, piece_y)) {
    piece_x--;
    tone(1820, 0x11);
  } else if (((pressed & J_RIGHT) ||
              ((keys & J_RIGHT) && hold > 6 && (hold & 1))) &&
             piece_x < W - 1 && fits(piece_x + 1, piece_y)) {
    piece_x++;
    tone(1820, 0x11);
  }
  if (pressed & J_A) {
    t = piece[2];
    piece[2] = piece[1];
    piece[1] = piece[0];
    piece[0] = t;
    tone(1880, 0x31);
  } else if (pressed & J_B) {
    t = piece[0];
    piece[0] = piece[1];
    piece[1] = piece[2];
    piece[2] = t;
    tone(1860, 0x31);
  }
  if (pressed & J_UP) {
    while (fits(piece_x, piece_y + 1)) {
      piece_y++;
      points(2);
    }
    lock();
    return;
  }
  if (fits(piece_x, piece_y + 1)) {
    lock_wait = 0;
    if (++fall_wait >= interval) {
      fall_wait = 0;
      piece_y++;
      if (keys & J_DOWN)
        points(1);
    }
  } else {
    fall_wait = 0;
    if (++lock_wait >= ((keys & J_DOWN) ? 2 : 12))
      lock();
  }
}
static uint8_t goal_met(void) {
  return well == 1 ? light >= LIGHT_GOAL : !stones;
}
void game_update(void) {
  busy = 0;
  if (banner_t)
    banner_t--;
  if (state == PLAY)
    control();
  else if (state == FLASH) {
    if (!(ticks & 1))
      redraw = 1;
    if (!--state_wait) {
      shatter();
      state = SETTLE;
    }
  } else if (state == SETTLE) {
    if (!settle()) {
      if (find_matches()) {
        chain++;
        state = FLASH;
        state_wait = 12;
        if (chain > 1) {
          banner = "CHAIN";
          banner_t = 40;
        }
      } else if (goal_met()) {
        state = CLEAR;
        state_wait = 70;
        points(1000 * well);
        banner = well == 5 ? "THE WELL SINGS!" : "WELL CLEAR!";
        banner_t = 70;
        tone(1980, 0xA7);
        chime(4);
      } else
        spawn();
    }
  } else if (!--state_wait) {
    if (++well > 5) {
      well = 5;
      ended = won = 1;
    } else
      setup_well();
  }
}
static int8_t landing(void) {
  int8_t y = piece_y;
  while (fits(piece_x, y + 1))
    y++;
  return y;
}
static void goals(void) {
  if (well == 1) {
    digits(13, 12, light > LIGHT_GOAL ? LIGHT_GOAL : light, 2, 2);
    label(15, 12, "/30", 4);
  } else
    digits(14, 12, stones, 2, stones ? 7 : 3);
  memset(tiles + 15 * 32 + 13, 0, 5);
  if (prism_in == 20 && next_piece[0] == PRISM)
    label(13, 15, "NEXT!", 2);
  else {
    label(13, 15, "IN", 4);
    digits(16, 15, prism_in, 2, 0);
  }
}
void game_draw(void) {
  uint8_t r, c, n, v, i, flash = (ticks & 2) != 0;
  int8_t ghost = state == PLAY ? landing() : -9, y;
  if (dirty) {
    backdrop();
    label(0, 0, "SCORE", 4);
    label(12, 0, "WELL", 4);
    digits(17, 0, well, 1, 1);
    label(18, 0, "/5", 4);
    label(13, 4, "NEXT", 4);
    label(13, 11, well == 1 ? "LIGHT" : "STONES", 4);
    label(13, 14, "PRISM", 4);
    shown_score = score + 1;
    dirty = 0;
    redraw = 1;
  }
  /* Rewrite the well only when it changes; 105 cells cost a third of a frame. */
  if (!busy && (redraw || ghost != shown_ghost || piece_x != shown_x)) {
    n = 0;
    for (r = 0; r < H; r++) {
      uint8_t *t = tiles + 66 + ((uint16_t)r << 5),
              *a = colors + 66 + ((uint16_t)r << 5);
      uint8_t shadow =
          ghost > piece_y + 2 && (int8_t)r >= ghost && (int8_t)r < ghost + 3;
      for (c = 0; c < W; c++, n++, t++, a++) {
        v = board[n];
        if (marks[n] && flash) {
          *t = FLASH_TILE;
          *a = 0;
        } else if (v) {
          *t = cell_tile[v];
          *a = cell_pal[v];
        } else if (shadow && c == piece_x) {
          *t = GHOST_TILE;
          *a = 4;
        } else
          *t = *a = 0;
      }
    }
    for (i = 0; i < 3; i++)
      tile(15, 5 + i, cell_tile[next_piece[i]], cell_pal[next_piece[i]]);
    goals();
    shown_ghost = ghost;
    shown_x = piece_x;
    redraw = 0;
  }
  if (score != shown_score) {
    digits(0, 1, score, 5, 1);
    shown_score = score;
  }
  if (state == PLAY) {
    v = fits(piece_x, piece_y + 1) ? (fall_wait << 3) / fall_speed[well] : 0;
    if (keys & J_DOWN)
      v = 0;
    for (i = 0; i < 3; i++) {
      y = piece_y + i;
      if (y >= 0)
        sprite(i, 15 + piece[i], (2 + piece_x) * 8, (2 + y) * 8 + v,
               cell_pal[piece[i]]);
    }
  }
  if (banner_t) {
    label(0, 17, banner, state == CLEAR ? 3 : 2);
    if (banner[0] == 'C' && banner[1] == 'H') {
      label(6, 17, "X", 2);
      digits(7, 17, chain, 1, 1);
    }
  } else
    label(0, 17, "A B CYCLE  UP DROP", 4);
}
