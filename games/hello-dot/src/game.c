#include "game.h"

static uint16_t random16(Game *g) {
    g->rng ^= (uint16_t)(g->rng << 7);
    g->rng ^= g->rng >> 9;
    g->rng ^= (uint16_t)(g->rng << 8);
    return g->rng;
}

static uint16_t distance(int16_t a, int16_t b) {
    return a > b ? (uint16_t)(a - b) : (uint16_t)(b - a);
}

static void add_score(Game *g, uint16_t points) {
    if (g->score > 65535u - points) g->score = 65535u;
    else g->score += points;
}

static void place_spark(Game *g, Spark *s) {
    uint8_t tries = 0;
    do {
        s->x = 13u + random16(g) % 134u;
        s->y = 32u + random16(g) % 88u;
        ++tries;
    } while (tries < 12u &&
             distance(s->x, (g->x >> 4) + 8) < 24u &&
             distance(s->y, (g->y >> 4) + 8) < 24u);
}

static void place_bug(Game *g, uint8_t index) {
    Bug *b = &g->bugs[index];
    uint8_t edge = random16(g) & 3u;
    uint8_t speed = 9u + (g->frame / 900u) * 2u;
    b->x = (int16_t)(12u + random16(g) % 136u) << 4;
    b->y = (int16_t)(31u + random16(g) % 89u) << 4;
    if (edge == 0u) b->x = 10 << 4;
    if (edge == 1u) b->x = 150 << 4;
    if (edge == 2u) b->y = 28 << 4;
    if (edge == 3u) b->y = 124 << 4;
    b->vx = b->x < g->x + 128 ? speed : -(int8_t)speed;
    b->vy = b->y < g->y + 128 ? speed - 2u : -(int8_t)(speed - 2u);
    b->wait = 55u;
}

void game_init(Game *g, uint16_t seed) {
    uint8_t i;
    g->x = 72 << 4;
    g->y = 72 << 4;
    g->face_x = 0;
    g->face_y = -1;
    g->frame = g->score = g->combo_timer = 0;
    g->rng = seed ? seed : 0xD07u;
    g->hearts = 3;
    g->dash = g->cooldown = g->invincible = 0;
    g->chain = g->charge = g->bursts = g->collected = g->popped = 0;
    g->multiplier = 1;
    g->flash = g->ended = g->event = g->last_input = 0;
    g->active_bugs = 2;
    for (i = 0; i < BUG_COUNT; ++i) place_bug(g, i);
    for (i = 0; i < SPARK_COUNT; ++i) {
        place_spark(g, &g->sparks[i]);
        g->sparks[i].wait = 0;
    }
    g->sparks[0].x = 80;
    g->sparks[0].y = 49;
    g->bugs[0].wait = g->bugs[1].wait = 100;
}

uint8_t game_seconds(const Game *g) {
    return g->frame >= ROUND_FRAMES ? 0 : (ROUND_FRAMES - g->frame + 59u) / 60u;
}

void game_tick(Game *g, uint8_t input) {
    uint8_t i, j;
    int8_t dx = 0, dy = 0;
    int16_t speed, px, py;
    g->event = 0;
    if (g->ended) return;
    ++g->frame;
    if (g->frame >= ROUND_FRAMES) {
        g->ended = 1;
        g->event = EVENT_END;
        return;
    }
    g->active_bugs = g->frame >= 1800u ? 4u : (g->frame >= 900u ? 3u : 2u);
    if (g->cooldown) --g->cooldown;
    if (g->dash) --g->dash;
    if (g->invincible) --g->invincible;
    if (g->flash) --g->flash;
    if (g->combo_timer && !--g->combo_timer) {
        g->chain = 0;
        g->multiplier = 1;
    }
    if (input & INPUT_UP) --dy;
    if (input & INPUT_DOWN) ++dy;
    if (input & INPUT_LEFT) --dx;
    if (input & INPUT_RIGHT) ++dx;
    if (!g->dash && (dx || dy)) {
        g->face_x = dx;
        g->face_y = dy;
    }
    if ((input & INPUT_DASH) && !(g->last_input & INPUT_DASH) && !g->cooldown) {
        g->dash = 11;
        g->cooldown = 48;
        g->event |= EVENT_DASH;
    }
    g->last_input = input;
    if (g->dash) {
        dx = g->face_x;
        dy = g->face_y;
        speed = dx && dy ? 56 : 80;
    } else if (input & INPUT_FOCUS) speed = dx && dy ? 8 : 11;
    else speed = dx && dy ? 16 : 23;
    g->x += dx * speed;
    g->y += dy * speed;
    if (g->x < 5 << 4) g->x = 5 << 4;
    if (g->x > 139 << 4) g->x = 139 << 4;
    if (g->y < 24 << 4) g->y = 24 << 4;
    if (g->y > 112 << 4) g->y = 112 << 4;
    px = (g->x >> 4) + 8;
    py = (g->y >> 4) + 8;

    for (i = 0; i < SPARK_COUNT; ++i) {
        Spark *s = &g->sparks[i];
        if (s->wait) { --s->wait; continue; }
        if (distance(px, s->x) < 11u && distance(py, s->y) < 11u) {
            if (g->chain < 28u) ++g->chain;
            g->multiplier = 1u + g->chain / 4u;
            g->combo_timer = 180;
            ++g->collected;
            add_score(g, 10u * g->multiplier);
            g->event |= EVENT_PICK;
            ++g->charge;
            place_spark(g, s);
            s->wait = 12;
            if (g->charge == 5u) {
                g->charge = 0;
                ++g->bursts;
                g->flash = 16;
                g->cooldown = 0;
                g->invincible = 35;
                add_score(g, 100u * g->multiplier);
                if (!(g->bursts & 1u) && g->hearts < 3u) ++g->hearts;
                for (j = 0; j < BUG_COUNT; ++j) place_bug(g, j);
                g->event |= EVENT_BURST;
            }
        }
    }

    for (i = 0; i < g->active_bugs; ++i) {
        Bug *b = &g->bugs[i];
        if (b->wait) { --b->wait; continue; }
        b->x += b->vx;
        b->y += b->vy;
        if (b->x <= 9 << 4 || b->x >= 151 << 4) b->vx = -b->vx;
        if (b->y <= 28 << 4 || b->y >= 125 << 4) b->vy = -b->vy;
        if (distance(px, b->x >> 4) < 9u && distance(py, b->y >> 4) < 9u) {
            if (g->dash) {
                ++g->popped;
                add_score(g, 25u * g->multiplier);
                g->combo_timer = 180;
                place_bug(g, i);
                g->event |= EVENT_POP;
            } else if (!g->invincible) {
                --g->hearts;
                g->invincible = 90;
                g->chain = g->charge = 0;
                g->multiplier = 1;
                g->combo_timer = 0;
                place_bug(g, i);
                g->event |= EVENT_HIT;
                if (!g->hearts) {
                    g->ended = 1;
                    g->event |= EVENT_END;
                    return;
                }
            }
        }
    }
}
