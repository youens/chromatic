#include "runtime.h"
#include <string.h>
const char game_title[] = "BLOOM CIRCUIT",
           game_tagline[] = "WAKE A LIVING WORLD",
           game_controls1[] = "D PAD CHOOSE TILE",
           game_controls2[] = "A ROTATE  B UNDO",
           game_goal[] = "CONNECT EVERY ROOT";
#define C RGB
const uint16_t game_palette[32] = {
    C(2, 6, 5),    C(6, 11, 9),   C(13, 20, 15), C(29, 29, 23), C(2, 6, 5),
    C(8, 13, 10),  C(20, 26, 15), C(30, 28, 17), C(2, 6, 5),    C(5, 15, 10),
    C(9, 26, 16),  C(22, 31, 21), C(2, 6, 5),    C(17, 8, 11),  C(27, 13, 19),
    C(31, 23, 24), C(2, 6, 5),    C(6, 11, 9),   C(10, 16, 12), C(20, 23, 17),
    C(2, 6, 5),    C(10, 14, 11), C(15, 21, 15), C(25, 28, 20), C(2, 6, 5),
    C(14, 11, 5),  C(24, 21, 10), C(31, 29, 18), C(2, 6, 5),    C(8, 13, 11),
    C(14, 23, 18), C(27, 31, 27)};
uint8_t board[42], solution[42], powered[42], visit[42], path[42],
    undo_cell[64], undo_value[64];
uint8_t width, height, cx, cy, lit, count, move_wait, undo_count, celebrate;
uint16_t moves;
static uint8_t rotate(uint8_t n) { return ((n << 1) | (n >> 3)) & 15; }
static uint8_t neighbor(uint8_t n, uint8_t d) {
  if (d == 0)
    return n >= width ? n - width : 255;
  if (d == 1)
    return n % width < width - 1 ? n + 1 : 255;
  if (d == 2)
    return n + width < count ? n + width : 255;
  return n % width ? n - 1 : 255;
}
static void power(void) {
  uint8_t head = 0, tail = 1, n, d, k;
  memset(powered, 0, sizeof(powered));
  path[0] = 0;
  powered[0] = 1;
  while (head < tail) {
    n = path[head++];
    for (d = 0; d < 4; d++) {
      k = neighbor(n, d);
      if (k != 255 && !powered[k] && (board[n] & (1 << d)) &&
          (board[k] & (1 << ((d + 2) & 3)))) {
        powered[k] = 1;
        path[tail++] = k;
      }
    }
  }
  lit = tail;
}
static void garden(void) {
  uint8_t top = 0, n, d, k, j, choices[4], r;
  width = stage + 2;
  if (width > 7)
    width = 7;
  height = width;
  if (height > 6)
    height = 6;
  count = width * height;
  memset(board, 0, sizeof(board));
  memset(visit, 0, sizeof(visit));
  visit[0] = 1;
  path[0] = 0;
  while (1) {
    n = path[top];
    j = 0;
    for (d = 0; d < 4; d++) {
      k = neighbor(n, d);
      if (k != 255 && !visit[k])
        choices[j++] = d;
    }
    if (!j) {
      if (!top)
        break;
      top--;
      continue;
    }
    d = choices[random16() % j];
    k = neighbor(n, d);
    board[n] |= 1 << d;
    board[k] |= 1 << ((d + 2) & 3);
    visit[k] = 1;
    path[++top] = k;
  }
  memcpy(solution, board, sizeof(board));
  for (n = 0; n < count; n++) {
    r = 1 + random16() % 3;
    while (r--)
      board[n] = rotate(board[n]);
  }
  cx = cy = undo_count = celebrate = move_wait = 0;
  power();
  if (lit == count) {
    board[0] = rotate(board[0]);
    power();
  }
}
void game_reset(void) {
  health = 0;
  moves = 0;
  garden();
}
void game_update(void) {
  uint8_t n;
  if (celebrate) {
    celebrate--;
    if (!celebrate) {
      stage++;
      if (stage > 5) {
        ended = won = 1;
      } else
        garden();
    }
    return;
  }
  if (move_wait)
    move_wait--;
  if (!move_wait) {
    if (keys & J_LEFT) {
      if (cx)
        cx--;
      move_wait = 5;
    } else if (keys & J_RIGHT) {
      if (cx + 1 < width)
        cx++;
      move_wait = 5;
    } else if (keys & J_UP) {
      if (cy)
        cy--;
      move_wait = 5;
    } else if (keys & J_DOWN) {
      if (cy + 1 < height)
        cy++;
      move_wait = 5;
    }
  }
  n = cy * width + cx;
  if (pressed & J_A) {
    if (undo_count == 64) {
      memmove(undo_cell, undo_cell + 1, 63);
      memmove(undo_value, undo_value + 1, 63);
      undo_count = 63;
    }
    undo_cell[undo_count] = n;
    undo_value[undo_count++] = board[n];
    board[n] = rotate(board[n]);
    moves++;
    power();
    tone(1640 + lit * 7, 0x63);
  } else if ((pressed & J_B) && undo_count) {
    undo_count--;
    n = undo_cell[undo_count];
    board[n] = undo_value[undo_count];
    cx = n % width;
    cy = n / width;
    moves++;
    power();
    tone(1450, 0x53);
  }
  if (lit == count) {
    points(1000 + count * 50);
    celebrate = 45;
    tone(1970, 0xA6);
  }
}
void game_draw(void) {
  uint8_t x, y, n, t, p, ox = (20 - width * 2) / 2, oy = 3;
  screen(FLOOR, 0);
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++) {
      n = y * width + x;
      t = PIPE_BASE + board[n] * 4;
      p = powered[n] ? 2 : 4;
      tile(ox + x * 2, oy + y * 2, t, p);
      tile(ox + x * 2 + 1, oy + y * 2, t + 1, p);
      tile(ox + x * 2, oy + y * 2 + 1, t + 2, p);
      tile(ox + x * 2 + 1, oy + y * 2 + 1, t + 3, p);
    }
  sprite(0, 15, (ox + cx * 2) * 8, (oy + cy * 2) * 8, 6);
  sprite(1, 15, (ox + cx * 2 + 1) * 8, (oy + cy * 2) * 8, 6 | S_FLIPX);
  sprite(2, 15, (ox + cx * 2) * 8, (oy + cy * 2 + 1) * 8, 6 | S_FLIPY);
  sprite(3, 15, (ox + cx * 2 + 1) * 8, (oy + cy * 2 + 1) * 8,
         6 | S_FLIPX | S_FLIPY);
  sprite(4, 8, ox * 8 + 4, oy * 8 + 4, 6);
  for (n = 0; n < count; n++)
    if (powered[n] &&
        (board[n] == 1 || board[n] == 2 || board[n] == 4 || board[n] == 8) &&
        n != cy * width + cx) {
      if (n % 4 == ticks / 8 % 4)
        sprite(5 + n % 8, 8, (ox + n % width * 2) * 8 + 4,
               (oy + n / width * 2) * 8 + 4, 3);
    }
  label(0, 0, "GARDEN", 4);
  number(7, 0, stage, 1, 1);
  label(12, 0, "MOVES", 4);
  number(15, 1, moves, 4, 1);
  label(0, 1, "ROOTS", 4);
  number(6, 1, lit, 2, 2);
  label(8, 1, "/", 4);
  number(9, 1, count, 2, 1);
  label(0, 17, celebrate ? "THE GARDEN AWAKENS!" : "A TURN     B UNDO",
        celebrate ? 2 : 4);
  if (!(ticks & 7)) {
    uint16_t c = C(18 + (ticks / 8 % 6), 31, 18);
    set_bkg_palette_entry(2, 3, c);
  }
}
