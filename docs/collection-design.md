# Nine games, nine kinds of play

The September 2026 anthology explores the Game Boy Color programming model
available to a compatible ModRetro Chromatic. These are first playable arcade
editions, not changes to the console's FPGA or firmware.

| Game | Core decision | Hardware or algorithm experiment | Completion |
| --- | --- | --- | --- |
| Neon Wake | Spend boost or preserve room to dodge? | Precomputed perspective roads, lane traffic, animated scenery | Survive a 60-second run |
| Moonthread | When should gravity reverse? | Fixed-point physics, inverted sprites, suspended ledges | Collect three stars in each of six chambers |
| Echo Vault | See the room or stay quiet? | Procedural maze, exploration memory, sonar, pathfinding guards | Escape four vaults |
| Bloom Circuit | Which root should turn next? | Generated solvable networks, live flood fill, palette animation | Connect five progressively larger gardens |
| Orbit Choir | Move now or wait for the strike window? | Polar lanes, timing windows, musical synthesis and combo scoring | Protect the planet for 60 seconds |
| Hello Dot | Chase a spark chain or dash through danger? | Timed combos, sprite animation, collision, burst effects | Maximize a 60-second score |
| Stormkite | Spend the gust now or save it for the boss? | LY=LYC scanline splits for three-band parallax, VBlank-timed weather palettes, a nine-sprite boss | Survive three waves and break the Thunderhead |
| Comet Links | Aim straight or let a planet bend the shot? | Offline-compiled per-tile force fields, deterministic fixed-point physics, an exact trajectory scope | Sink nine holes (par 31) |
| Prism Well | Clear now or build toward a chain? | Four-direction match scanning, animated cascades, colourblind-safe glyphs, wave-channel chimes | Clear five wells, four of them floored with stone |

Stormkite, Comet Links and Prism Well each have a full design document:
[Stormkite](../games/stormkite/DESIGN.md), [Comet Links](../games/comet-links/DESIGN.md)
and [Prism Well](../games/prism-well/DESIGN.md). They fill the genres the first
six left open: a shooter, a sports game and a falling-block puzzle. Each also
uses hardware the others do not: scanline interrupts and hardware scrolling,
precompiled physics, and the wave channel.

## Scope of the hardware work

The eight shared-runtime cartridges use double-speed CGB CPU mode and DMA
tile-map transfers. Their intended game update rate is 30 Hz; the screen refreshes at
native hardware rate. Static backgrounds and the road projection are prebuilt
to avoid expensive per-frame division. Each cartridge is 32 KiB with no mapper,
external RAM, runtime download, or battery save requirement.

The experiments push beyond the first Hello Dot loop into different rendering
and simulation techniques. They do not exhaust everything the hardware can do.
The anthology cartridge adds MBC5 banking and battery-backed saves for its
launcher. Link play, infrared, sampled music, and custom FPGA work are possible
future projects, not implemented features of this collection.

The update budget is one display frame, not two. The runtime waits for the
next VBlank after each update, so an update that runs past one frame costs a
third frame. The newer games keep inside it by redrawing only what changed,
writing decimal digits by subtraction, stepping indices instead of
multiplying, and spreading the Comet Links scope over several updates. The
tests measure this.

## Art direction

Every game has a distinct color palette, cartridge character design, title
screen, and cover. The first five shared-runtime covers were generated with the
built-in image tool and are stored inside the corresponding game folder. The
Stormkite, Comet Links and Prism Well covers are pixel posters composed by
[../tools/pixel_covers.py](../tools/pixel_covers.py) from each cartridge's own
tiles, sprites and palette. The prompt and
provenance accompany each cover. Gameplay uses separately authored native 2bpp
pixel art. The website labels actual screenshots separately from cover art.

| Saved cover | Final generation prompt |
| --- | --- |
| [Neon Wake](../games/neon-wake/art/cover.jpg) | [Prompt](../games/neon-wake/art/README.md) |
| [Moonthread](../games/moonthread/art/cover.jpg) | [Prompt](../games/moonthread/art/README.md) |
| [Echo Vault](../games/echo-vault/art/cover.jpg) | [Prompt](../games/echo-vault/art/README.md) |
| [Bloom Circuit](../games/bloom-circuit/art/cover.jpg) | [Prompt](../games/bloom-circuit/art/README.md) |
| [Orbit Choir](../games/orbit-choir/art/cover.jpg) | [Prompt](../games/orbit-choir/art/README.md) |

The built-in image generation tool produced all five illustrations. JPEG
encoding reduces their combined size from about 15 MB to 3 MB while preserving
the composition and dimensions. Native gameplay screenshots remain PNG files.

## Website and verification

`chromatic.youens.com` hosts the collection, nine dedicated play pages and a page
that runs the whole anthology cartridge with its launcher.
Each uses the same downloadable ROM in binjgb. The collection includes keyboard,
touch, and standard gamepad input, fullscreen, audio control, restart, individual
ROM downloads, and a ZIP of all nine cartridges plus the anthology. The private GitHub repository
contains source; production uploads only the generated static distribution.

The PyBoy integration suite checks cartridge headers, checksums, boot flow,
help, pause, update rate, puzzle solutions, full rounds, and progression.
See [collection-validation.json](collection-validation.json) for results.
Actual hardware testing remains pending.

## Release checks, September 29, 2026

Stormkite, Comet Links and Prism Well each completed a full run with joypad
input in PyBoy, at the full 30 Hz update rate. Stormkite's three bands shifted
2, 4 and 8 pixels over eight frames while its HUD stayed fixed. Comet Links
matched the Python physics model exactly at the end of every stroke. The
nine-game cartridge launched every game twice. It kept a real Neon Wake best
score across an emulated power cycle, and ran in Chrome through binjgb.

Earlier checks for the first six games:

All five new cartridges completed their full campaigns or timed rounds using
joypad input in PyBoy. Moonthread preserves collected stars after a collision
and grants a recovery window; Echo Vault allows 800 moves for four vaults.
The original Hello Dot logic and ROM validation suite also passes.

Chrome launched all six ROMs through the collection player. Native rendering
was visually checked, along with the library filter and a 390 × 844 phone
layout. Physical controllers and the Chromatic itself have not been tested.
