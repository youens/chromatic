/* Three-band parallax using LY=LYC interrupts. The background map holds one
   seamless 256-pixel harbor; each band reads it with its own SCX value. The
   HUD rows above and below the bands stay fixed at SCX 0. */
#include <gb/cgb.h>
#include <gb/gb.h>
#include "sky.h"
extern uint8_t phase;
uint8_t sky_flash, sky_mood, sky_speed = 4;
static uint8_t band, installed, shown = 255;
static uint8_t scroll[3];
static uint16_t travel[3];
/* Interrupt one line early, then wait for HBlank before changing SCX. */
static const uint8_t split[5] = {15, 55, 95, 135, 255};
#define N RGB(3, 3, 10)
static const uint16_t moods[4][12] = {
    {N, RGB(9, 6, 18), RGB(21, 10, 19), RGB(30, 24, 26), N, RGB(10, 4, 12),
     RGB(26, 11, 9), RGB(31, 24, 8), N, RGB(4, 5, 13), RGB(7, 14, 20),
     RGB(29, 22, 9)},
    {N, RGB(6, 6, 17), RGB(12, 10, 23), RGB(26, 26, 30), N, RGB(8, 4, 12),
     RGB(20, 9, 12), RGB(31, 24, 8), N, RGB(3, 4, 12), RGB(6, 11, 19),
     RGB(31, 25, 10)},
    {N, RGB(6, 7, 12), RGB(11, 12, 18), RGB(22, 24, 28), N, RGB(6, 4, 10),
     RGB(15, 8, 12), RGB(28, 20, 8), N, RGB(3, 4, 9), RGB(5, 9, 14),
     RGB(24, 18, 8)},
    {RGB(16, 16, 25), RGB(22, 22, 29), RGB(27, 27, 31), RGB(31, 31, 31), N,
     RGB(12, 8, 16), RGB(28, 18, 14), RGB(31, 28, 16), RGB(9, 9, 18),
     RGB(8, 9, 20), RGB(14, 18, 26), RGB(31, 29, 18)}};
static void sky_line(void) {
  uint8_t x = band < 3 ? scroll[band] : 0;
  while (STAT_REG & 3)
    ;
  SCX_REG = x;
  LYC_REG = split[++band];
}
static void sky_frame(void) {
  uint8_t i, want = 0;
  SCX_REG = 0;
  band = 0;
  if (phase == 2 || phase == 3) {
    LYC_REG = split[0];
    if (phase == 2) {
      travel[0] += sky_speed;
      travel[1] += sky_speed << 1;
      travel[2] += sky_speed << 2;
    }
    for (i = 0; i < 3; i++)
      scroll[i] = travel[i] >> 4;
    want = sky_mood;
    if (sky_flash) {
      sky_flash--;
      want = 3;
    }
  } else
    LYC_REG = 255;
  if (want != shown) {
    set_bkg_palette(5, 3, moods[want]);
    shown = want;
  }
}
void sky_start(void) {
  if (installed)
    return;
  disable_interrupts();
  add_VBL(sky_frame);
  add_LCD(sky_line);
  LYC_REG = 255;
  STAT_REG = STATF_LYC;
  installed = 1;
  enable_interrupts();
  set_interrupts(VBL_IFLAG | LCD_IFLAG);
}
void sky_stop(void) {
  if (!installed)
    return;
  set_interrupts(VBL_IFLAG);
  disable_interrupts();
  remove_LCD(sky_line);
  remove_VBL(sky_frame);
  STAT_REG = 0;
  SCX_REG = 0;
  installed = 0;
  shown = 255;
  sky_flash = 0;
  enable_interrupts();
}
