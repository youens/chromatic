/* The launcher's fixed-bank part. The shelf's scanline split runs from
   interrupts, which must never find their handlers banked out; everything
   else in the launcher lives in a switchable bank (menu.c). */
#include <gb/gb.h>
#include <stdint.h>

extern uint8_t band_scx, band_on;
uint8_t band_split;
void launcher_main(void) BANKED;

/* LY=LYC: scroll the icon band, then restore the details below it. */
void shelf_line(void) {
  uint8_t x = band_split ? 0 : band_scx;
  while (STAT_REG & 3)
    ;
  SCX_REG = x;
  LYC_REG = band_split ? 255 : 79;
  band_split = 1;
}

/* VBlank: the header starts unscrolled every frame. */
void shelf_frame(void) {
  SCX_REG = 0;
  band_split = 0;
  LYC_REG = band_on ? 15 : 255;
}

void main(void) {
  launcher_main();
}
