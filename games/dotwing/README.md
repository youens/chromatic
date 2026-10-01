# Dotwing

Your dot. Your sky. A fast vertical flying shooter for Game Boy Color and
ModRetro Chromatic, written in C and SM83 assembly with GBDK 4.5.0. The
browser player runs the same cartridge. One cartridge, many games.

Build a DevDay-inspired co-pilot, board your plane, and fly four scrolling
sectors to take on Grok, Claude, Gemini and Muse. Each lab sends its own
emblem fleet and a giant boss drawn across the background layer: a ringed
orbital slash, a coral sunburst, twin crystal stars and a purple M
battleship. These are original pixel interpretations. Muse's M is a
placeholder until the intended lab/logo is identified. This is an independent
fan game with offline arcade opponents; it does not make AI requests.

## Play

On your first launch, A or Start opens the dot builder. Up/Down chooses
shape, color, face or gear; Left/Right changes it. A or Start saves; B
cancels your edits. Your dot rides in the cockpit and colors the plane's
livery.

| Button | In flight |
| --- | --- |
| D-pad | Steer two pixels a frame, including diagonals |
| Hold A | Focus: one pixel a frame for threading bullets. Guns always fire |
| B | Token burst: spends 50 charge, clears every hostile bullet, shields you briefly and hits every rival on screen |
| Start | Pause; Start resumes, B banks your tokens and ends the sortie |
| Select | Toggle sound |
| Start + Select | Return to the launcher in the collection cartridge |

Rivals drop pickups:

- **Token coins** add one token (two during a burst) and 25 points.
- **Power** raises the guns one level for the rest of the sortie (up to two;
  a hit costs one).
- **Repair** restores a heart.
- **Bolt** refills burst charge and doubles the fire rate for eight seconds.
- **Wing dot** (some heavy craft carry one) adds an ally that trails your
  plane and fires with you, up to two per sortie.

Defeats in quick succession build a chain that multiplies their points up
to eight times. Taking a hit costs a heart and a power level, resets the
chain and gives two seconds of invincibility.

Each sector streams about eighty seconds of scenery and scripted
squadrons: drones dive, swoop and snake, gunners hover and aim, and heavy
craft fire rings. Then a warning sounds and the lab's boss slides in. Every
boss changes its attack at two thirds and one third of its hull. Clearing
a sector banks a bonus and opens the sky station, which repairs one heart
and sells upgrades before the next sector. Defeat the fourth boss to earn AI
supremacy. Failed sorties keep every token you collected, so the next
flight starts stronger.

| Sector | Rival | Scenery | Boss attacks |
| --- | --- | --- | --- |
| 1 Azure Isles | Grok | Floating grass islands and clouds over a blue sky | Aimed fans, rotating rings, spiral storm |
| 2 Coral Drift | Claude | Coral rock mesas in a violet sky | Sunburst rings, ring and fan mixes, twin spirals |
| 3 Crystal City | Gemini | Floating neon city blocks at night | Alternating star volleys, crossing streams, twin rings |
| 4 Violet Nexus | Muse | Crystal shards in a dark nebula | V-shaped walls, aimed wing volleys, dense needle spiral |

## Upgrades

The hangar and sky station sell four permanent upgrades. Each has three
levels costing 40, 80 and 140 tokens.

| Upgrade | Effect per level |
| --- | --- |
| Wing gun | Faster fire, then angled side shots, then twin shots that hit harder. Power pickups stack on top |
| Hull | One more heart (three to six) |
| Reactor | Faster burst recharge, a stronger burst and a wider pickup magnet |
| Ally dots | One wing dot from launch, then two, then two that fire twin shots |

## Saves

The ROM uses MBC5 with battery-backed 8 KiB cartridge RAM. Avatar, upgrade
levels, tokens, cleared sectors and best score use two alternating 32-byte
checksummed records at SRAM A200/A220. A record is committed last so an
incomplete write can fall back to its predecessor. Saves from the first
release load unchanged. This area is separate from the collection's
launcher records.

The browser saves cartridge RAM to local storage separately for each game;
the anthology keeps its own save. Clearing site storage removes that browser
save. The standalone and anthology save files are independent. A physical
cartridge needs compatible battery-backed RAM for persistence.

## Build

```sh
make -C games/dotwing all
make -C games/dotwing test
make -C games/dotwing release
make -C games/arcade release
make -C collection build
```

Run from the repository root with GBDK installed at `.tools/gbdk` and the
pinned PyBoy/Pillow dependencies in `.tools/venv`. `GBDK_HOME` and `PYTHON`
can override the standalone tool paths. Every pixel is placed by the Python
in `tools/`: `tools/assets.py` writes `src/art_ids.h`, `src/gfx_data.h`,
`src/terraina_data.h`, `src/terrainb_data.h` and `src/boss_data.h`, and
`tools/music.py` compiles the songs in `tools/songs.py` into
`src/music_data.h`. The generated headers are committed so the cartridge
builds without Python. The standalone cartridge is `dist/dotwing.gbc`
(128 KiB, GBC-only).

The screen never flashes white: after power-on the LCD stays on, and every
screen change fades the palettes to black, rebuilds video memory and fades
back in. Logic runs once per display frame in double-speed mode. See
`DESIGN.md` for the engine.

`make test` drives the real ROM in PyBoy and records the exact ROM hash,
ordinary button-only play (including a bot campaign), controlled
collision, pickup, boss and save scenarios, LCD and frame-timing checks in
`build/validation.json`. Emulator checks do not establish physical
cartridge behavior or audible sound quality.
