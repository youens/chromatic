/* Chromatic Arcade launcher: a sliding shelf of nine cartridges, info cards,
   and battery-backed records. The icon band scrolls on its own scanline split
   while the header and the game details stay still. */
#include "runtime.h"
#include "digits.h"
#include "launcher-data.h"
#include "modules.h"
#include <string.h>

#define HELLO_KIND 255
#define INTRO 0
#define SHELF 1
#define INFO 2
#define RECORDS 3
#define SAVE_VERSION 2
#define ERASE_FRAMES 120

typedef struct {
  uint8_t magic[4];
  uint8_t version, last;
  uint16_t best[GAME_COUNT], plays[GAME_COUNT];
  uint16_t check;
} Records;

Records records;
uint8_t arcade_selected, arcade_in_game, menu_index, launcher_mode, saves_ok;
uint8_t center_slot, ring_game[4], slide_dir, slide_frame, fade_level;
uint8_t band_scx, band_on, erase_hold, frame_count;
static uint8_t band_split, installed, map_dirty, previous, held_for;
static uint16_t pal_mask;
static uint16_t palettes[64], faded[64];
static const uint8_t slide_steps[8] = {14, 12, 10, 8, 7, 6, 4, 3};
static const char magic[4] = {'C', 'H', 'R', 'A'};
#define INK RGB(2, 2, 6)
static const uint16_t base_palettes[64] = {
    /* 0 text, 1 accent, 2-5 shelf slots, 6 dim, 7 brand */
    INK, RGB(8, 9, 16), RGB(14, 15, 24), RGB(30, 29, 26),
    INK, RGB(6, 6, 12), RGB(12, 12, 20), RGB(30, 29, 26),
    INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK,
    INK, RGB(6, 6, 12), RGB(10, 10, 18), RGB(17, 18, 26),
    INK, RGB(20, 29, 27), RGB(26, 22, 30), RGB(28, 21, 15),
    /* sprites: 0 selection frame, 1 NEW badge, 2 arrows */
    INK, RGB(6, 6, 12), RGB(14, 15, 24), RGB(30, 29, 26),
    INK, RGB(31, 24, 8), RGB(31, 24, 8), RGB(5, 3, 8),
    INK, RGB(12, 13, 22), RGB(12, 13, 22), RGB(12, 13, 22),
    INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK, INK,
    INK, INK, INK, INK};

/* ------------------------------------------------------------- records */
static uint16_t checksum(void) {
  const uint8_t *p = (const uint8_t *)&records;
  uint16_t sum = 0x5A17;
  uint8_t i;
  for (i = 0; i < sizeof(Records) - 2; i++)
    sum = ((sum << 1) | (sum >> 15)) ^ p[i];
  return sum;
}
static void save_records(void) {
  records.check = checksum();
  ENABLE_RAM;
  SWITCH_RAM(0);
  memcpy((void *)0xA000, &records, sizeof(Records));
  saves_ok = !memcmp((void *)0xA000, &records, sizeof(Records));
  DISABLE_RAM;
}
static void reset_records(void) {
  memset(&records, 0, sizeof(Records));
  memcpy(records.magic, magic, 4);
  records.version = SAVE_VERSION;
}
static void load_records(void) {
  ENABLE_RAM;
  SWITCH_RAM(0);
  memcpy(&records, (void *)0xA000, sizeof(Records));
  DISABLE_RAM;
  if (memcmp(records.magic, magic, 4) || records.version != SAVE_VERSION ||
      records.check != checksum() || records.last >= GAME_COUNT)
    reset_records();
  save_records();
}

/* ------------------------------------------------------ screen helpers */
static void flush_rows(void) {
  /* The 18 visible rows of both maps: 36 blocks of 16 bytes each. */
  VBK_REG = 0;
  HDMA1_REG = 0xD0;
  HDMA2_REG = 0;
  HDMA3_REG = 0x18;
  HDMA4_REG = 0;
  HDMA5_REG = 35;
  VBK_REG = 1;
  HDMA1_REG = 0xD4;
  HDMA2_REG = 0;
  HDMA3_REG = 0x18;
  HDMA4_REG = 0;
  HDMA5_REG = 35;
  VBK_REG = 0;
}
static void prepare_palettes(void) {
  uint8_t i;
  uint16_t c;
  const uint8_t *scale = fade_table + fade_level * 32;
  if (fade_level == 8) {
    memcpy(faded, palettes, sizeof(faded));
    return;
  }
  for (i = 0; i < 64; i++) {
    c = palettes[i];
    faded[i] = scale[c & 31] | (uint16_t)scale[(c >> 5) & 31] << 5 |
               (uint16_t)scale[(c >> 10) & 31] << 10;
  }
}
/* One VBlank carries either palette changes or a map transfer, never both. */
static void frame(void) {
  uint8_t i;
  vsync();
  if (pal_mask) {
    for (i = 0; i < 8; i++) {
      if (pal_mask & (1 << i))
        set_bkg_palette(i, 1, faded + i * 4);
      if (pal_mask & (0x100 << i))
        set_sprite_palette(i, 1, faded + 32 + i * 4);
    }
    pal_mask = 0;
  } else if (map_dirty) {
    flush_rows();
    map_dirty = 0;
  }
  frame_count++;
}
static void palette(uint8_t n, const uint16_t *colors4) {
  memcpy(palettes + n * 4, colors4, 8);
  pal_mask |= n < 8 ? 1 << n : 0x100 << (n - 8);
  prepare_palettes();
}
static void fade_to(uint8_t level) {
  while (fade_level != level) {
    fade_level += fade_level < level ? 1 : -1;
    prepare_palettes();
    pal_mask = 0xFFFF;
    frame();
  }
}
static void clear_row(uint8_t y) {
  memset(tiles + ((uint16_t)y << 5), 0, 20);
  memset(colors + ((uint16_t)y << 5), 0, 20);
}
static void text(uint8_t x, uint8_t y, const char *s, uint8_t pal) {
  while (*s && x < 20)
    tile(x++, y, (uint8_t)*s++ - 31, pal);
}
static void centered(uint8_t y, const char *s, uint8_t pal) {
  clear_row(y);
  text((20 - strlen(s)) >> 1, y, s, pal);
}
static void draw_icon(uint8_t x, uint8_t y, uint8_t game, uint8_t pal) {
  uint8_t r, c;
  const uint8_t *t = icon_tiles + (uint16_t)game * 36,
                *a = icon_attrs + (uint16_t)game * 36;
  for (r = 0; r < 6; r++)
    for (c = 0; c < 6; c++)
      tile(x + c, y + r, *t++, *a++ | pal);
}
static void header(void) {
  uint8_t x;
  tile(1, 0, UI_BRAND, 7);
  tile(2, 0, UI_BRAND + 1, 7);
  tile(1, 1, UI_BRAND + 2, 7);
  tile(2, 1, UI_BRAND + 3, 7);
  text(4, 0, "CHROMATIC FUN", 0);
  text(4, 1, "BY YOUENS", 6);
}
static void hints(const char *a, const char *b, const char *select) {
  clear_row(17);
  tile(0, 17, UI_BTN_A, 0);
  text(2, 17, a, 6);
  tile(7, 17, UI_BTN_B, 0);
  text(9, 17, b, 6);
  if (select) {
    tile(14, 17, UI_BTN_SEL, 0);
    text(16, 17, select, 6);
  }
}
static uint8_t wrap(int8_t n) {
  return n < 0 ? n + GAME_COUNT : n >= GAME_COUNT ? n - GAME_COUNT : n;
}
static void sound_blip(uint16_t f, uint8_t sweep) {
  NR10_REG = sweep;
  NR11_REG = 0x80;
  NR12_REG = 0x61;
  NR13_REG = (uint8_t)f;
  NR14_REG = 0x80 | (f >> 8);
}
static void sound_chord(uint16_t f) {
  NR21_REG = 0x80;
  NR22_REG = 0x74;
  NR23_REG = (uint8_t)f;
  NR24_REG = 0x80 | (f >> 8);
}

/* ------------------------------------------------------------- the shelf */
static void shelf_line(void) {
  uint8_t x = band_split ? 0 : band_scx;
  while (STAT_REG & 3)
    ;
  SCX_REG = x;
  LYC_REG = band_split ? 255 : 79;
  band_split = 1;
}
static void shelf_frame(void) {
  SCX_REG = 0;
  band_split = 0;
  LYC_REG = band_on ? 15 : 255;
}
static void install(void) {
  if (installed)
    return;
  disable_interrupts();
  add_VBL(shelf_frame);
  add_LCD(shelf_line);
  LYC_REG = 255;
  STAT_REG = STATF_LYC;
  installed = 1;
  enable_interrupts();
  set_interrupts(VBL_IFLAG | LCD_IFLAG);
}
static void uninstall(void) {
  if (!installed)
    return;
  set_interrupts(VBL_IFLAG);
  disable_interrupts();
  remove_LCD(shelf_line);
  remove_VBL(shelf_frame);
  STAT_REG = 0;
  SCX_REG = 0;
  installed = 0;
  enable_interrupts();
}
static void set_slot(uint8_t slot, uint8_t game) {
  ring_game[slot] = game;
  draw_icon(slot * 8 + 1, 3, game, 2 + slot);
  palette(2 + slot, icon_palettes + game * 4);
  map_dirty = 1;
}
static void set_accent(void) {
  uint16_t c[4];
  c[0] = INK;
  c[1] = RGB(6, 6, 12);
  c[2] = RGB(12, 12, 20);
  c[3] = accents[menu_index];
  palette(1, c);
  c[1] = RGB(8, 8, 14);
  c[2] = RGB(16, 16, 24);
  palette(8, c);
}
static void shelf_details(void) {
  uint8_t i;
  const uint16_t plays = records.plays[menu_index];
  clear_row(10);
  for (i = 0; i < GAME_COUNT; i++)
    tile(5 + i, 10, i == menu_index ? UI_DOT_ON : UI_DOT,
         i == menu_index ? 1 : 6);
  centered(12, game_names[menu_index], 0);
  centered(13, game_genres[menu_index], 1);
  centered(14, "START+SELECT: HOME", 7);
  clear_row(16);
  if (!plays)
    centered(16, "NEW CARTRIDGE", 7);
  else {
    text(0, 16, "BEST", 6);
    digits(5, 16, records.best[menu_index], 5, 0);
    text(11, 16, "PLAYS", 6);
    digits(17, 16, plays, 3, 0);
  }
  set_accent();
  map_dirty = 1;
}
static void draw_shelf(void) {
  uint8_t x, y, k;
  screen(0, 0);
  header();
  for (y = 2; y < 10; y++)
    for (x = 0; x < 32; x++)
      if (((x & 7) * 3 + y * 5) % 13 == 0)
        tile(x, y, (x + y) & 1 ? UI_STAR : UI_STAR2, 6);
  center_slot = 1;
  for (k = 0; k < 4; k++)
    set_slot(k, wrap((int8_t)menu_index + k - 1));
  band_scx = center_slot * 64 - 48;
  band_on = 1;
  shelf_details();
  hints("PLAY", "INFO", "HALL");
}
static void shelf_sprites(void) {
  uint8_t pulse = (frame_count >> 4) & 1, nudge = slide_dir ? 2 : 0;
  set_sprite_tile(0, SPR_CORNER);
  set_sprite_tile(1, SPR_CORNER);
  set_sprite_tile(2, SPR_CORNER);
  set_sprite_tile(3, SPR_CORNER);
  set_sprite_prop(0, 0);
  set_sprite_prop(1, S_FLIPX);
  set_sprite_prop(2, S_FLIPY);
  set_sprite_prop(3, S_FLIPX | S_FLIPY);
  move_sprite(0, 8 + 53 - pulse, 16 + 21 - pulse);
  move_sprite(1, 8 + 99 + pulse, 16 + 21 - pulse);
  move_sprite(2, 8 + 53 - pulse, 16 + 67 + pulse);
  move_sprite(3, 8 + 99 + pulse, 16 + 67 + pulse);
  if (!slide_dir && !records.plays[menu_index]) {
    set_sprite_tile(4, SPR_NEW_L);
    set_sprite_tile(5, SPR_NEW_R);
    set_sprite_prop(4, 1);
    set_sprite_prop(5, 1);
    move_sprite(4, 8 + 86, 16 + 19);
    move_sprite(5, 8 + 94, 16 + 19);
  } else {
    move_sprite(4, 0, 0);
    move_sprite(5, 0, 0);
  }
  set_sprite_tile(6, SPR_ARROW);
  set_sprite_tile(7, SPR_ARROW);
  set_sprite_prop(6, 2);
  set_sprite_prop(7, 2 | S_FLIPX);
  move_sprite(6, 8 + 1 - (slide_dir == 0xFF ? nudge : 0), 16 + 44);
  move_sprite(7, 8 + 151 + (slide_dir == 1 ? nudge : 0), 16 + 44);
}
static void hide_sprites(void) {
  uint8_t i;
  for (i = 0; i < 40; i++)
    move_sprite(i, 0, 0);
}
static void start_slide(int8_t dir) {
  set_slot((center_slot + 2) & 3, wrap((int8_t)menu_index + dir * 2));
  slide_dir = (uint8_t)dir;
  slide_frame = 0;
  sound_blip(dir > 0 ? 1840 : 1800, 0x14);
}
static void slide(void) {
  int8_t dir = (int8_t)slide_dir;
  band_scx += dir * slide_steps[slide_frame];
  if (slide_frame == 3) {
    menu_index = wrap((int8_t)menu_index + dir);
    shelf_details();
  }
  if (++slide_frame == 8) {
    center_slot = (center_slot + dir) & 3;
    slide_dir = 0;
  }
}

/* ------------------------------------------------- intro, info, records */
static void draw_intro(void) {
  uint8_t x, y;
  screen(0, 0);
  for (y = 0; y < 18; y++)
    for (x = 0; x < 20; x++)
      if ((x * 7 + y * 11) % 23 == 0)
        tile(x, y, (x + y) & 1 ? UI_STAR : UI_STAR2, 6);
  tile(9, 2, UI_BRAND, 7);
  tile(10, 2, UI_BRAND + 1, 7);
  tile(9, 3, UI_BRAND + 2, 7);
  tile(10, 3, UI_BRAND + 3, 7);
  for (y = 0; y < 6; y++)
    for (x = 0; x < 20; x++)
      tile(x, y + 5, menu_logo_map[y * 20 + x], 0);
  centered(12, "BY YOUENS", 6);
  centered(17, "START+SELECT: HOME", 6);
  band_on = 0;
}
static void draw_info(void) {
  const char *const *help = game_help + menu_index * 3;
  screen(0, 0);
  centered(0, game_names[menu_index], 0);
  centered(1, game_genres[menu_index], 1);
  draw_icon(7, 3, menu_index, 2);
  palette(2, icon_palettes + menu_index * 4);
  centered(10, game_blurbs[menu_index * 2], 0);
  centered(11, game_blurbs[menu_index * 2 + 1], 0);
  text(1, 13, help[0], 6);
  text(1, 14, help[1], 6);
  text(1, 15, help[2], 1);
  if (records.plays[menu_index]) {
    text(0, 16, "BEST", 6);
    digits(5, 16, records.best[menu_index], 5, 0);
    text(11, 16, "PLAYS", 6);
    digits(17, 16, records.plays[menu_index], 3, 0);
  }
  hints("PLAY", "BACK", 0);
  band_on = 0;
  map_dirty = 1;
}
static void draw_records(void) {
  uint8_t i;
  uint16_t total = 0;
  screen(0, 0);
  header();
  centered(2, "HALL OF LIGHT", 7);
  for (i = 0; i < GAME_COUNT; i++) {
    text(1, 4 + i, game_names[i], i == menu_index ? 1 : 0);
    if (records.plays[i])
      digits(15, 4 + i, records.best[i], 5, i == menu_index ? 1 : 6);
    else
      text(17, 4 + i, "---", 6);
    total += records.plays[i];
  }
  text(1, 14, "GAMES PLAYED", 6);
  digits(15, 14, total, 5, 0);
  centered(15, saves_ok ? "SAVED ON CARTRIDGE" : "NO SAVE MEMORY", 6);
  centered(16, "HOLD A+B TO ERASE", 6);
  clear_row(17);
  tile(0, 17, UI_BTN_B, 0);
  text(2, 17, "BACK", 6);
  band_on = 0;
  map_dirty = 1;
}
static void erase_meter(void) {
  uint8_t i, n = erase_hold / (ERASE_FRAMES / 10);
  for (i = 0; i < 10; i++)
    tile(5 + i, 16, i < n ? UI_BAR : UI_BAR_OFF, i < n ? 1 : 6);
  map_dirty = 1;
}
static void show(uint8_t mode) {
  launcher_mode = mode;
  slide_dir = 0;
  hide_sprites();
  if (mode == INTRO)
    draw_intro();
  else if (mode == SHELF)
    draw_shelf();
  else if (mode == INFO)
    draw_info();
  else
    draw_records();
  map_dirty = 1;
}

/* --------------------------------------------------- entering and games */
static void enter(uint8_t mode) {
  DISPLAY_OFF;
  uninstall();
  VBK_REG = 0;
  SCX_REG = SCY_REG = 0;
  HIDE_WIN;
  SPRITES_8x8;
  hide_sprites();
  NR52_REG = 0;
  NR52_REG = 0x80;
  NR50_REG = 0x77;
  NR51_REG = 0xFF;
  launcher_art();
  memcpy(palettes, base_palettes, sizeof(palettes));
  fade_level = 0;
  prepare_palettes();
  set_bkg_palette(0, 8, faded);
  set_sprite_palette(0, 8, faded + 32);
  show(mode);
  flush_rows();
  map_dirty = 0;
  pal_mask = 0;
  SHOW_BKG;
  SHOW_SPRITES;
  install();
  DISPLAY_ON;
  fade_to(8);
}
static void launch(void) {
  uint8_t kind = game_kind[menu_index], slot = menu_index;
  if (records.plays[slot] < 999)
    records.plays[slot]++;
  records.last = slot;
  save_records();
  sound_blip(1900, 0x16);
  sound_chord(1850);
  hide_sprites();
  fade_to(0);
  while (joypad())
    frame();
  uninstall();
  DISPLAY_OFF;
  NR52_REG = 0;
  arcade_in_game = 1;
  if (kind == HELLO_KIND) {
    best_score = records.best[slot];
    hello_run();
    if (best_score > records.best[slot])
      records.best[slot] = best_score;
  } else {
    arcade_selected = kind;
    best = records.best[slot];
    arcade_metadata();
    arcade_run();
    if (best > records.best[slot])
      records.best[slot] = best;
  }
  arcade_cleanup();
  arcade_in_game = 0;
  save_records();
  enter(SHELF);
  while (joypad())
    frame();
  previous = 0;
}
void main(void) {
  uint8_t keys, pressed;
  cpu_fast();
  load_records();
  menu_index = records.last;
  enter(INTRO);
  sound_blip(1750, 0);
  for (;;) {
    frame();
    keys = joypad();
    pressed = keys & ~previous;
    previous = keys;
    held_for = keys ? held_for + 1 : 0;
    if (frame_count == 12 && launcher_mode == INTRO)
      sound_chord(1797);
    if (frame_count == 24 && launcher_mode == INTRO)
      sound_blip(1860, 0);
    if (launcher_mode == INTRO) {
      if (!(frame_count & 15)) {
        if (frame_count & 16)
          clear_row(14);
        else
          centered(14, "PRESS START", 1);
        map_dirty = 1;
      }
      if (pressed & (J_START | J_A)) {
        sound_blip(1880, 0x15);
        fade_to(0);
        enter(SHELF);
      }
    } else if (launcher_mode == SHELF) {
      if (slide_dir)
        slide();
      else if (keys & (J_RIGHT | J_DOWN))
        start_slide(1);
      else if (keys & (J_LEFT | J_UP))
        start_slide(-1);
      else if (pressed & (J_A | J_START)) {
        launch();
        continue;
      } else if (pressed & J_B) {
        sound_blip(1760, 0);
        show(INFO);
      } else if (pressed & J_SELECT) {
        sound_blip(1720, 0);
        show(RECORDS);
      }
      if (launcher_mode == SHELF)
        shelf_sprites();
    } else if (launcher_mode == INFO) {
      if (pressed & (J_A | J_START)) {
        launch();
        continue;
      }
      if (pressed & J_B) {
        sound_blip(1700, 0);
        show(SHELF);
      }
    } else {
      if ((keys & (J_A | J_B)) == (J_A | J_B)) {
        if (erase_hold < ERASE_FRAMES && ++erase_hold == ERASE_FRAMES) {
          reset_records();
          save_records();
          draw_records();
          centered(16, "RECORDS ERASED", 1);
          NR41_REG = 0x10;
          NR42_REG = 0xA3;
          NR43_REG = 0x55;
          NR44_REG = 0x80;
        } else if (erase_hold < ERASE_FRAMES)
          erase_meter();
      } else {
        if (erase_hold && erase_hold < ERASE_FRAMES)
          draw_records();
        erase_hold = 0;
        if (pressed & J_B) {
          sound_blip(1700, 0);
          show(SHELF);
        }
      }
    }
  }
}
