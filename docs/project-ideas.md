# Things we can build

These are design proposals based on the [capability research](chromatic-capabilities.md),
not existing software or promises of untested hardware support.

## Strong first projects

| Idea | Why it fits | First playable scope |
| --- | --- | --- |
| Pocket dungeon | Tile maps, compact turn-based state, reusable enemy art | One floor, three enemy types, a key, an exit, and saved best score |
| One-screen arcade game | Small art budget and immediately testable controls | One room, one movement mechanic, rising difficulty, restart |
| Tiny garden | Small persistent state and palette-driven visual changes | Three plants and progress measured in play time; add real-world time only with an RTC cart |
| Chiptune instrument | Direct use of the four-channel sound unit | D-pad selects notes, A/B play, Start switches patterns |
| Pixel sketchbook | Small tile canvas and limited palette | Draw, select colors, save one picture on a suitable cart |
| Generative art cartridge | Tiles and palette cycling create variety cheaply | Seed selection, three patterns, pause, and screenshot capture on the Mac |

My first choice is a **one-screen dungeon or arcade game in GBDK**. It will teach
us the complete path from code to a physical cartridge while leaving enough room
for personality, polish, and sound. Finish a five-minute experience before
expanding the world.

## Projects that exploit the connectors

| Idea | Needed beyond the ROM | Key engineering question |
| --- | --- | --- |
| Head-to-head puzzle duel | Two handhelds and a compatible link cable | How do we synchronize inputs and recover from disconnects? |
| IR collectible exchange | Two IR-capable handhelds | Can a short packet transfer reliably with alignment and retries? |
| Cooperative dungeon | Link cable and two copies of the game | Which machine owns state, and how is desynchronization detected? |
| Pocket music duet | Link cable | Can one device provide reliable tempo and pattern synchronization? |
| Game Boy Printer art toy | Compatible printer and cable | Does this exact printer/protocol work with the Chromatic? |
| Desktop-connected adventure | Custom link adapter or a documented future host API | What is the verified bidirectional transport? |

Implement a tiny message protocol before building a multiplayer game around it.
For a first link game, a turn-based design keeps bandwidth and synchronization
demands manageable. Printer and custom-adapter ideas need accessory validation.

## AI-assisted and computer-assisted ideas

**Offline story cartridge:** generate and edit branching dialogue on the computer,
then compile the final text into the ROM. The device needs no network and the
story works untethered. Memory and UI readability set the practical scope.

**Seeded challenge packs:** generate levels on the computer or from a compact
in-game seed. Share seeds as text, through IR, or by link cable. This can create
large replay value without storing an image for every level.

**Live companion experiment:** a computer supplies dialogue or events through a
verified bidirectional bridge while the handheld renders the game. This could
support cloud-backed characters, but the bridge is a separate project. USB
capture alone only supplies game output; `live-demo` alone does not establish a
network API accessible to a cartridge ROM.

**Stream overlay:** computer software consumes captured gameplay, adds a themed
frame or scoreboard, and records demos. Start with manual or image-based state
detection unless we deliberately implement a telemetry transport.

## Advanced work for later

- A diagnostic cartridge for input, audio, palettes, save memory, link, and IR.
- System-menu or accessibility changes through open MCU/FPGA firmware.
- An SD-backed homebrew library using verified compatible custom firmware.
- A purpose-built cartridge with sensors, rumble, an RTC, or another interface.
- Wireless experiments after verifying the actual board and firmware interfaces.
- A new FPGA core after establishing resource limits and a supported toolchain.

These projects can be rewarding, but none is necessary to make an excellent
original game. Our first milestone should remain a reproducible build, a fun
interaction, and a cartridge that boots without the computer.
