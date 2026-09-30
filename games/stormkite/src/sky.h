#ifndef STORMKITE_SKY_H
#define STORMKITE_SKY_H
#include <stdint.h>
/* Scanline-split parallax. The handlers must stay in the fixed ROM bank. */
extern uint8_t sky_flash, sky_mood, sky_speed;
void sky_start(void);
void sky_stop(void);
#endif
