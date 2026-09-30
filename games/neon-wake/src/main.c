#include "runtime.h"
#include <string.h>
extern const uint8_t road_maps[], road_colors[];
uint8_t dirty;
int8_t previous_curve;
const char game_title[] = "NEON WAKE", game_tagline[] = "OUTRUN THE NIGHT",
           game_controls1[] = "LEFT RIGHT STEER",
           game_controls2[] = "A BOOST  B BRAKE",
           game_goal[] = "SURVIVE 60 SECONDS";
#define C RGB
const uint16_t game_palette[32] = {
    C(2, 2, 6),    C(5, 5, 12),   C(12, 15, 23), C(29, 30, 31), C(2, 2, 6),
    C(9, 5, 19),   C(30, 4, 23),  C(9, 31, 31),  C(2, 2, 6),    C(1, 10, 15),
    C(2, 25, 30),  C(22, 31, 31), C(2, 2, 6),    C(18, 4, 10),  C(30, 9, 19),
    C(31, 23, 10), C(2, 2, 6),    C(8, 7, 15),   C(18, 13, 25), C(25, 24, 31),
    C(3, 3, 8),    C(4, 4, 10),   C(17, 6, 24),  C(9, 30, 31),  C(2, 2, 6),
    C(6, 6, 14),   C(11, 10, 22), C(22, 9, 25),  C(2, 2, 6),    C(10, 4, 15),
    C(24, 8, 21),  C(31, 19, 12)};
typedef struct {
  int16_t z;
  int8_t lane;
  uint8_t gem;
} Traffic;
Traffic cars[6];
int16_t steering;
uint16_t distance_run;
uint8_t boost, fuel, invincible, speed;
int8_t curve;
const int8_t turns[16] = {0, 1,  2,  3,  4,  3,  2,  1,
                          0, -1, -2, -3, -4, -3, -2, -1};
void reset_car(uint8_t i) {
  cars[i].z = -(int16_t)(random16() % 140);
  cars[i].lane = (int8_t)(random16() % 3) - 1;
  cars[i].gem = (random16() & 3) == 0;
}
void game_reset(void) {
  uint8_t i;
  dirty = 1;
  previous_curve = 127;
  steering = 0;
  distance_run = 0;
  fuel = 100;
  invincible = boost = 0;
  for (i = 0; i < 6; i++) {
    reset_car(i);
    cars[i].z = -(int16_t)i * 32;
  }
}
void game_update(void) {
  uint8_t i;
  int16_t x;
  curve = turns[(ticks / 75) & 15];
  if (invincible)
    invincible--;
  boost = (keys & J_A) && fuel > 1;
  speed = (keys & J_B) ? 1 : (boost ? 4 : 2);
  if (boost)
    fuel -= 2;
  else if (fuel < 100)
    fuel++;
  if (keys & J_LEFT)
    steering -= boost ? 4 : 3;
  if (keys & J_RIGHT)
    steering += boost ? 4 : 3;
  if (!(ticks & 3))
    steering -= curve > 0 ? 1 : (curve < 0 ? -1 : 0);
  if (steering < -64)
    steering = -64;
  if (steering > 64)
    steering = 64;
  if (distance(steering, 0) > 51) {
    speed = 1;
    if (!(ticks % 15))
      noise(0x37);
  }
  distance_run += speed;
  points(speed);
  for (i = 0; i < 6; i++) {
    cars[i].z += speed;
    if (cars[i].z >= 91 && cars[i].z < 99) {
      x = cars[i].lane * 31;
      if (distance(steering, x) < 13) {
        if (cars[i].gem) {
          points(150);
          fuel = 100;
          tone(1920, 0xA3);
          cars[i].z = 130;
        } else if (!invincible) {
          health--;
          invincible = 40;
          fuel /= 2;
          noise(0x67);
          cars[i].z = 130;
        }
      }
    }
    if (cars[i].z > 110) {
      if (!cars[i].gem)
        points(40);
      reset_car(i);
    }
  }
  if (!(ticks % 30))
    tone(boost ? 1850 : 1650, 0x22);
  if (!health)
    ended = 1;
  if (ticks >= 1800) {
    ended = won = 1;
  }
}
void game_draw(void) {
  uint8_t i, pips = fuel / 10;
  int16_t z, sx, sy;
  const uint8_t *r =
      road_maps + (uint16_t)(curve + 4) * 640 + ((distance_run >> 2) & 1) * 320;
  const uint8_t *c = road_colors + (uint16_t)(curve + 4) * 320;
  if (dirty) {
    backdrop();
    dirty = 0;
  }
  memcpy(tiles + 224, r, 320);
  if (previous_curve != curve) {
    memcpy(colors + 224, c, 320);
    previous_curve = curve;
  }
  for (i = 0; i < 6; i++) {
    z = cars[i].z;
    if (z < 0 || z > 105)
      continue;
    sy = 53 + (((uint16_t)z * 5) >> 3);
    sx =
        80 + curve * ((105 - z) >> 5) + cars[i].lane * (7 + ((uint16_t)z >> 2));
    if (cars[i].gem)
      sprite(4 + i * 4, 14, sx - 4, sy, 3);
    else if (z < 45)
      sprite(4 + i * 4, 9, sx - 4, sy, 3);
    else
      actor(4 + i * 4, 4, sx - 8, sy - 5, 3, 0);
  }
  if (!invincible || (ticks & 2))
    actor(0, 0, 72 + steering, 112, 1, 0);
  if (boost) {
    sprite(30, 8, 73 + steering, 130, 2);
    sprite(31, 8, 83 + steering, 130, 2);
  }
  if (!(ticks & 1))
    hud("TIME", 60 - ticks / 30);
  label(0, 17, boost ? "BOOST" : "DRIVE", 2);
  for (i = 0; i < 10; i++)
    tile(7 + i, 17, i < pips ? LINE : VOID, 2);
}
