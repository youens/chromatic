# Dotwing design

The fast movement and readable attack lanes of 1942 and Sky Force inspire an
offline arcade flight game with an AI theme. A friendly customizable dot
anchors the game's identity. Every sector is a place: floating islands,
coral mesas, a neon city and a crystal nebula scroll past in layers, and
each rival lab brings its own colors, fleet and boss.

The loop is build a dot, outfit a plane, launch, dodge and chain rival
defeats, collect boosts, defeat a sector boss, upgrade at a sky station,
and challenge the next lab. Collected tokens persist even after a failed
run. Four sectors complete a campaign; best score gives replay value.

| Sector | Rival fleet | Boss | Scenery |
| --- | --- | --- | --- |
| 1 Azure Isles | Grok: graphite craft marked with a slash | Ringed sphere with a slash | Grass islands and cumulus over a blue sky |
| 2 Coral Drift | Claude: coral sunburst craft | Petal sunburst | Rock mesas and drifting cloud in a violet sky |
| 3 Crystal City | Gemini: indigo four-point stars | Twin crystal stars on a bridge | Floating city blocks with lit windows at night |
| 4 Violet Nexus | Muse: violet M craft | M battleship | Crystal shards in a dark nebula |

Each lab fields three craft: drones that dive, swoop, weave and cross the
screen; gunners that hover and aim; and heavy craft that fire rings and
carry power-ups or wing dots. Waves are scripted per sector against the
scroll position, so a sector plays the same way every time and can be
learned. Bosses have three attack phases each, switching at two thirds and
one third of their hull, and sway as they fire.

Focus movement (hold A) and a small hit core make dense patterns fair. The
B burst is a timed escape that turns danger into points: it clears every
bullet, grants brief invincibility and damages everything on screen.

## Never flash white

The first release turned the LCD off between screens, which shows as a
white flash on a Game Boy Color. Now the LCD stays on from the first title
frame until the cartridge powers down. A screen change fades the CGB
palettes to black over eight frames, rewrites tiles and maps while the
screen is black (writes wait for VRAM access, so nothing tears), then fades
back in. Pause dims the scenery palettes instead of hiding them. The
emulator test fails if the LCD is ever off after boot.

## Frame structure

The game runs in double-speed mode and updates once per display frame:

1. Read the joypad and run the state's logic. Flight logic moves the
   plane, fires, spawns scripted squadrons, moves rivals, shots, bullets
   and pickups, resolves collisions and writes this frame's sprites into
   the OAM buffer that DMA is not reading.
2. Hand the finished sprite buffer to the VBlank OAM DMA, mark the video
   buffers ready and wait for VBlank. At the start of the interrupt, upload
   changed palette colours and run the queued CGB general-purpose DMA
   transfers for scenery, boss and HUD rows. An interrupt during unfinished
   game logic leaves these buffers alone.
3. The same fixed-bank VBlank hook then runs the sound driver and restores
   the interrupted ROM bank. After the wait, apply the scroll registers.
   Video transfers must precede sound so they finish before drawing resumes.

Positions are 8.8 fixed point whose high byte is the screen pixel plus 32,
so every on-screen coordinate is one byte with no shifting. The hot loops
(bullets against the plane, shots against packed rival boxes, sprite
emitters) are hand-written SM83 assembly. The HUD redraws one element per
frame into a buffer and reaches VRAM by DMA. In a full button-only bot
campaign under the emulator, under one percent of display frames overrun.

## Scenery and bosses

A level is 24 columns by 300 rows. Each sector's art is drawn in Python
from shapes (islands, clouds, towers, crystals) into stamps; tiles are
deduplicated with horizontal and vertical flips and each tile picks one of
five scenery palettes. A sector uses about 150 to 175 tiles in both VRAM
banks. Rows stream into the 32-row background map just above the screen as
the camera climbs, built from a hashed base pattern with stamp cells
copied over it. The background is 32 columns wide and the camera follows
the plane sideways by up to 14 pixels for parallax.

Bosses are 96 by 56 pixel paintings on the background layer: their tiles
load into VRAM bank 1 at sector start, the scenery is flattened to the
sector's sky colour during the warning, and the boss rows are written above
the screen. Scrolling the whole background then moves the boss smoothly
while sprites carry its glowing core and the rival bullets. When a boss
dies its palettes ease into the sky before the scenery returns.

## Hardware budget

| Resource | Use |
| --- | --- |
| Background palettes | 0-5 scenery or boss (5 also colors sky text), 6-7 HUD |
| Sprite palettes | Plane livery, player energy, rival bullets, rival body, rival accent, fire, gold, shadow and repair |
| Sprites | 8x16 mode; the plane, allies, flame, up to 8 rivals, 12 player shots, 24 bullets, 6 pickups and 10 effects share 40 objects |
| Window | Two-row HUD at the bottom; the pause panel raises it |
| Upper work RAM | D000-DBFF: row and HUD buffers, the second OAM buffer, actor pools, palettes, ally trail, scenery streaming state and the sector's layout tables |

The standalone cartridge uses six switchable banks: the main loop and sound
driver share bank 1 with sectors 1-2, the menus share bank 2 with sectors
3-4, flight has bank 3, loaders and title art bank 4, shared helpers and
the rival AI bank 5, and scenery streaming with the bosses bank 6. The
anthology keeps these exact pairings in banks 26-31, and Dotwing's upper
work RAM is time-shared with the other games because only the running game
uses it.

## Sound

`tools/music.py` compiles a compact MML dialect (loops, repeats, ties,
instruments, drum kit) into byte streams for the two pulse channels, the
wave channel and noise. Songs cover the title, hangar, two flight themes,
boss, sector clear, victory and game over; the compiler checks that every
channel of a song loops in sync. Sound effects borrow channel 1 or 4 by
priority and hand them back to the music.

## Saves and progression

Customization and progression use a versioned, bounds-checked two-slot
save with a rolling generation and checksum. Tokens are credited once per
run checkpoint; leaving a shop cannot repeat its reward. Byte 19 holds the
ally upgrade, which was always zero in the first release, so old saves load
as they were. Cartridge save regions do not overlap. Browser SRAM is
namespaced by game slug and restored before boot.
