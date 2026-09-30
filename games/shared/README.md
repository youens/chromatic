# Chromatic cartridge runtime

The eight shared-runtime games share menus, text rendering, sound helpers,
input, scoring, and display transfer code. Each game supplies its own rules, palette,
art direction, controls, and rendering.

## Display and timing

The games use Game Boy Color double-speed CPU mode and a 30 Hz logic target
(one update per two native display frames). The display itself runs at the
hardware's approximately 59.73 Hz rate. A nominal sixty-second round is 1,800
logic updates, about 60.3 seconds when the target rate is maintained.

Tile and attribute maps occupy two aligned 1 KiB buffers at WRAM addresses
`0xD000` and `0xD400`, in the default switchable WRAM bank. Do not switch that
bank or allocate another object over those buffers. General-purpose CGB DMA
copies both maps during VBlank. The tile maps use the hardware's 32-cell row
stride, even though only 20 × 18 cells are visible.

Static starfields and planets are prebuilt. Neon Wake additionally uses lookup
tables for its road bends, replacing expensive per-pixel arithmetic. The games
use 8 × 8 hardware sprites; 16 × 16 characters are assembled from four sprites.

The software stays within 32 KiB ROM, without mapper, cartridge RAM, firmware
changes, or networking. Standalone best scores reset when the cartridge resets;
the anthology cartridge saves them. Physical Chromatic testing is pending.

The runtime's `number()` divides once per digit, which costs several scanlines
on the Game Boy CPU. The newer games use `digits.h`, which subtracts powers of
ten instead, and redraw numbers only when they change.

## Art

`assets.py` creates deterministic native 2bpp graphics, including a distinct hero
for each game, 16 circuit connection shapes, perspective road maps, tile-based
planets, scenery, text and title logos. `worlds.py` adds the backgrounds and
sprites of Stormkite, Comet Links and Prism Well. Those three games do not use
the pipe tiles, so background slots 76–191 (except the planet) are theirs;
repeated blocks and their flips are deduplicated into those slots. These are the actual game graphics.
The larger illustrated covers are separate presentation artwork.

## Build and verification

Each project includes `game.mk`; a game can list extra C files in `EXTRA`.
Run `make games` from the repository root to rebuild every release ROM. `tools/test_collection.py` boots the compiled
cartridges, reads exported symbols for assertions, and supplies real joypad
input. It does not alter the emulated game state. Its puzzle solver can read the
hidden solution, and the maze solver can read hidden tiles, so those tests prove
mechanical reachability rather than human difficulty.

Runtime library licensing is covered by the GBDK linking exception at
[../hello-dot/licenses/GBDK.txt](../hello-dot/licenses/GBDK.txt).
