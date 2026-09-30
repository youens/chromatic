#ifndef CHROMATIC_RUNTIME_H
#define CHROMATIC_RUNTIME_H
#include <gb/cgb.h>
#include <gb/gb.h>
#include <stdint.h>
#define FLOOR 60
#define WALL 61
#define LINE 62
#define STAR 63
#define SPIKE 64
#define GATE 65
#define LEAF 66
#define ORB 67
#define DITHER 68
#define EDGE_L 69
#define EDGE_R 70
#define DASH 71
#define WATER 72
#define BRICK 73
#define HEART 74
#define VOID 75
#define PIPE_BASE 96
extern uint8_t __at(0xD000) tiles[1024];
extern uint8_t __at(0xD400) colors[1024];
extern uint8_t keys, pressed, phase, health, stage, won, ended;
extern uint16_t ticks, score, best, seed;
extern const char game_title[], game_tagline[], game_controls1[],
    game_controls2[], game_goal[];
extern const uint16_t game_palette[32];
void game_reset(void);
void game_update(void);
void game_draw(void);
void screen(uint8_t tile, uint8_t pal);
void backdrop(void);
#define tile(tx, ty, tt, pp)                                                   \
  do {                                                                         \
    uint16_t tile_index = ((uint16_t)(ty) << 5) + (tx);                        \
    tiles[tile_index] = (tt);                                                  \
    colors[tile_index] = (pp);                                                 \
  } while (0)
void label(uint8_t x, uint8_t y, const char *s, uint8_t pal);
void number(uint8_t x, uint8_t y, uint16_t value, uint8_t width, uint8_t pal);
void actor(uint8_t id, uint8_t t, int16_t x, int16_t y, uint8_t pal,
           uint8_t flip);
void sprite(uint8_t id, uint8_t t, int16_t x, int16_t y, uint8_t pal);
void hide_all(void);
void tone(uint16_t frequency, uint8_t envelope);
void noise(uint8_t pitch);
void points(uint16_t n);
uint16_t random16(void);
uint16_t distance(int16_t a, int16_t b);
void hud(const char *right, uint16_t value);
#endif
