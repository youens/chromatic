#ifndef HELLO_DOT_GAME_H
#define HELLO_DOT_GAME_H

#include <stdint.h>

#define INPUT_UP 1u
#define INPUT_DOWN 2u
#define INPUT_LEFT 4u
#define INPUT_RIGHT 8u
#define INPUT_DASH 16u
#define INPUT_FOCUS 32u
#define EVENT_PICK 1u
#define EVENT_DASH 2u
#define EVENT_HIT 4u
#define EVENT_POP 8u
#define EVENT_BURST 16u
#define EVENT_END 32u
#define SPARK_COUNT 4u
#define BUG_COUNT 4u
#define ROUND_FRAMES 3600u

typedef struct {
    int16_t x, y;
    int8_t vx, vy;
    uint8_t wait;
} Bug;

typedef struct {
    uint8_t x, y, wait;
} Spark;

typedef struct {
    int16_t x, y;
    int8_t face_x, face_y;
    uint16_t frame, score, rng, combo_timer;
    uint8_t hearts, dash, cooldown, invincible;
    uint8_t chain, multiplier, charge, bursts, collected, popped;
    uint8_t flash, ended, active_bugs, event, last_input;
    Bug bugs[BUG_COUNT];
    Spark sparks[SPARK_COUNT];
} Game;

void game_init(Game *g, uint16_t seed);
void game_tick(Game *g, uint8_t input);
uint8_t game_seconds(const Game *g);

#endif
