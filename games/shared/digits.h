#ifndef CHROMATIC_DIGITS_H
#define CHROMATIC_DIGITS_H
/* Decimal output by repeated subtraction. The runtime's number() divides
   once per digit, which costs several scanlines on the Game Boy CPU. Like
   number(), it shows only the lowest `width` digits. Include once per game. */
static const uint16_t decimal_powers[5] = {10000, 1000, 100, 10, 1};
static void digits(uint8_t x, uint8_t y, uint16_t v, uint8_t width,
                   uint8_t pal) {
  uint8_t i, d;
  uint16_t p;
  for (i = 0; i < 5; i++) {
    p = decimal_powers[i];
    d = 0;
    while (v >= p) {
      v -= p;
      d++;
    }
    if (i >= 5 - width)
      tile(x++, y, 17 + d, pal);
  }
}
#endif
