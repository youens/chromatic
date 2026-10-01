/* The only fixed-bank code Dotwing adds: a VBlank hook that maps the sound
   driver's bank, runs one tick and restores whatever bank was mapped. */
#include "dotwing.h"

BANKREF_EXTERN(dw_sound)

void dw_vbl(void) {
  uint8_t saved = _current_bank;
  SWITCH_ROM(BANK(dw_sound));
  dw_sound_tick();
  SWITCH_ROM(saved);
}
