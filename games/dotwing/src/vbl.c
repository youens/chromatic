/* The fixed-bank VBlank hook finishes queued video updates before sound,
   then restores whichever bank the interrupted game code was using. */
#include "dotwing.h"

BANKREF_EXTERN(dw_sound)

void dw_vbl(void) {
  uint8_t saved = _current_bank;
  dw_video_tick();
  SWITCH_ROM(BANK(dw_sound));
  dw_sound_tick();
  SWITCH_ROM(saved);
}
