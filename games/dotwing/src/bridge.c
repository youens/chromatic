/* Read caller-bank strings before the renderer switches to its helper bank. */
#include "dotwing.h"

char __at(0xDF00) dw_text[21];
void dw_label_buffer(uint8_t x, uint8_t y, uint8_t palette) BANKED;

void dw_label(uint8_t x, uint8_t y, const char *text, uint8_t palette) {
  uint8_t i = 0;
  char c;
  while (i < 20) {
    c = *text++;
    if (!c)
      break;
    dw_text[i++] = c;
  }
  dw_text[i] = 0;
  dw_label_buffer(x, y, palette);
}
