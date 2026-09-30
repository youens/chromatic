# Stormkite design

**Genre:** horizontal parallax shooter · **Length:** about 90 seconds a flight ·
**Core question:** spend the gust now, or save it for the Thunderhead?

A storm with a mind of its own rolls over a harbor town's lantern festival. You
fly a paper kite above the rooftops, pop storm spirits with paper darts, catch
drifting lanterns, and finally break the Thunderhead that drives the storm.

## Pillars

1. **The world moves.** Three background layers slide at three speeds. The
   sky, the city and the rooftops each scroll from the same background map on
   their own scanline split. This is the collection's first hardware-scrolling
   game.
2. **Readable danger at 30 Hz.** Every threat is a bright yellow or lilac shape
   on a dark sky. Bolts are aimed, but never faster than 3 pixels per update.
   The kite's hitbox is a 6 × 6 core inside its 16 × 16 sprite.
3. **One meaningful resource.** The gust meter is the only thing you manage.
   It clears the screen, but a charge spent early is not there for the boss.

## Controls

| Input | Action |
| --- | --- |
| D-pad | Fly in eight directions, 2 px per update |
| A (hold) | Fire paper darts; one every 4 updates, at most 4 on screen |
| B | Release the gust when the meter is full |
| Start | Pause |
| Select | Sound |

## The flight

| Wave | Starts | Sky | New pressure |
| --- | --- | --- | --- |
| 1 Dusk | 0 s | Violet sunset | Storm wisps drift in sine waves; a line of three every 140 updates |
| 2 Nightfall | 20 s | Blue night | Squalls (16 × 16, three hits) park on the right and fire aimed bolts |
| 3 Squall line | 45 s | Grey storm, lightning | Swifts dive toward your altitude; wind shear pushes the kite up or down |
| 4 Thunderhead | 70 s | Storm | The boss: 48 hits, bolt fans that widen to five at half health, summons wisps |

Wind shear is announced on the status line 30 updates before it arrives
("WIND RISING" or "DOWNDRAFT"). It then pushes 1 px per update for 75 updates.
The kite flies at 2 px, so you can always fly against it.

## Economy

| Event | Points | Gust |
| --- | --- | --- |
| Wisp | 50 | +1 |
| Swift | 80 | +2 |
| Squall | 150 | +3 |
| Lantern | 100 | +8 |
| Dart on the Thunderhead | 10 | — |
| Gust on the Thunderhead | 80 | −8 boss health |
| Thunderhead defeated | 2,000 + 300 per heart | — |

The meter holds 24 units, shown as eight pips. A full gust clears every bolt.
It defeats every wisp and swift on screen and takes two hits off each squall.
It also gives 30 updates of invulnerability. You have five hearts, and each
hit grants 60 updates of invulnerability.

## Art and sound

The harbor is one seamless 256-pixel-wide picture. `worlds.py` paints it and
deduplicates it into 60 tiles, reusing flipped tiles. The picture's
repetition is tile-aligned: building windows fall on tile boundaries and roof
slopes share a period. The first pixel row of every band is empty, so a split
that lands one line late cannot be seen. Palettes change in vertical blank:
violet dusk, blue night, then grey storm. Lightning briefly brightens the sky
band.

Sound uses the shared pulse and noise helpers: soft ticks for darts, a
two-note lantern chime, low thunder and a noise sweep for the gust.

## Hardware technique

`src/sky.c` installs a VBlank handler and an LY=LYC handler. At VBlank it sets
SCX to 0 for the HUD and advances three sub-pixel scroll counters: far,
city and rooftops move at 1×, 2× and 4× the base rate. It then arms LYC for
line 15. Each LYC interrupt waits for HBlank, writes the next band's SCX and
arms the next split: lines 55, 95 and finally 135, where SCX returns to 0 for
the status row. The handlers read the runtime's `phase`. They scroll only
while a flight is running or paused, so the title, help and results screens
stay still.

In the anthology cartridge these handlers are compiled into the fixed bank,
since an interrupt can arrive while any game bank is mapped. The launcher
calls `sky_stop()` when you return to the shelf.

## Budgets

| Resource | Use |
| --- | --- |
| ROM | 32 KiB cartridge, 16.1 KiB used |
| Work RAM | 156 bytes |
| Background tiles | 60 of 100 free slots |
| Sprites | kite 4, tail 1, darts 4, bolts 5, lantern 1, bursts 2, foes 20; the 9-sprite Thunderhead reuses three foe slots |
| Frame time | The measured update rate stays at 30 Hz through the boss fight. The HUD redraws only changed digits, and spawners use countdown timers instead of 16-bit modulo |

## Verification

`tools/test_collection.py` compares screen captures taken eight frames apart.
It checks that the HUD rows stay fixed while the three bands shift by 2, 4
and 8 pixels. A joypad-driven bot then flies the whole storm. It must beat the
Thunderhead while averaging at least 98% of the 30 Hz update target. The bot
can read every bolt's position, so this proves the flight can be completed,
not that it is easy.
