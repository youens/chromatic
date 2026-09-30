# Chromatic capabilities

Researched **September 29, 2026**. Target: ModRetro Chromatic. The owner is
collecting a unit at OpenAI DevDay and expects a programming session. Its exact
hardware revision, cartridge, activation entitlement, and accessories are not
yet known. This is a source-based guide, not a report of testing our unit.

## What we can build

The dependable starting point is a **Game Boy Color ROM**: a compact game or
application that runs from a compatible cartridge. It can use color pixel art,
scrolling backgrounds, animated sprites, four-channel music, saved progress,
link-cable communication, and infrared. Monochrome Game Boy software is also a
valid target. ModRetro explicitly welcomes new game development.
[Compatibility](https://modretro.com/products/chromatic-tetris-bundle),
[developer invitation](https://modretro.com/blogs/blog/create-for-chromatic).

There are three distinct development layers:

| Layer | What we write | What it enables |
| --- | --- | --- |
| Game/application | GB/GBC ROM, usually C, assembly, or GB Studio | Games, music tools, pixel art tools, simulations, local multiplayer |
| Computer companion | Desktop software communicating with the handheld | Capture, cartridge tools, and official host-emulated live demos |
| System firmware | FPGA logic and ESP32 firmware | Changes to the console itself; requires hardware knowledge and recovery planning |

The official CLI now advertises both cartridge writing and `live-demo`, which
emulates a ROM on the computer and streams its video/audio to the Chromatic.
The public firmware sources offer a separate route to deeper modifications.
[CLI](https://www.npmjs.com/package/@modretro/chromatic-cli),
[FPGA source](https://github.com/ModRetro/oss-chromatic-console-fpga),
[MCU source](https://github.com/ModRetro/oss-chromatic-console-mcu).

## Physical hardware and everyday features

| Area | Capability | Development consequence |
| --- | --- | --- |
| Screen | 160 × 144 pixels, 2.56-inch backlit IPS LCD | Design at native resolution; readable text and strong silhouettes matter |
| Controls | D-pad, A, B, Start, Select; separate system Menu button and volume dial | Eight digital game inputs; Menu is a system control, not an extra standard game button |
| Audio | Built-in speaker and 3.5 mm headphone output | Use headphones to evaluate stereo; speaker mutes when headphones are inserted |
| Cartridge slot | GB, GBC, and Chromatic cartridges | Cartridge electrical behavior, mapper, and capacity must match the ROM |
| Link port | Connects compatible GB/GBC/Chromatic systems | Two-player games and custom serial protocols |
| Infrared | Transmit and receive between compatible systems | Small exchanges without a cable |
| USB-C | External power, firmware updates, gameplay capture | One cable can support a practical desktop development setup |
| Power | Three AA cells, USB power, or optional Rechargeable Power Core | USB powers the device without consuming the installed battery |
| Materials | Retail versions offer Gorilla Glass or sapphire; metal shell and PBT controls | Exact DevDay enclosure and screen-cover material remain unconfirmed |

Sources: [product specifications](https://modretro.com/products/chromatic-tetris-bundle),
[hardware tour](https://support.modretro.com/en_us/welcome-to-chromatic-know-your-chromatic-SJS5feAZg),
[power options](https://support.modretro.com/en_us/welcome-to-chromatic-power-options-ryhxbg_Rx).

USB charging applies to the supported Power Core. Do not assume that installing
rechargeable AA cells makes the console an AA charger. Actual battery runtime
depends on the cells, brightness, cartridge, and workload; we have not measured it.

The status LED indicates low battery with red blinking and Power Core charging
with a steady white light; it goes out when charging completes. The console also
has an on-screen battery indicator and automatic low-power brightness behavior.
[LED guide](https://support.modretro.com/en_us/welcome-to-chromatic-led-indicator-rJZ4WxuRWe),
[power settings](https://support.modretro.com/en_us/welcome-to-chromatic-system-settings-SkHcfgu0Zl).

## The game-programming budget

These are **GB/GBC programming limits**, not specifications for all the modern
chips inside the Chromatic. Standard cartridge software sees the compatible
console environment, not unrestricted access to the ESP32 or FPGA resources.

| Resource | Game Boy mode | Game Boy Color mode |
| --- | --- | --- |
| CPU model | 8-bit Sharp SM83-family instruction set | Same instruction set |
| CPU clock | About 4.19 MHz | About 4.19 MHz or 8.39 MHz in double-speed mode |
| Work RAM | 8 KiB | 32 KiB, partly banked |
| Video RAM | 8 KiB | 16 KiB in two banks |
| Visible image | 160 × 144 | 160 × 144 |
| Display cadence | About 59.73 frames/second | About 59.73 frames/second |
| Hardware sprites | 40 total; at most 10 intersecting a scanline | Same |
| Sprite dimensions | 8 × 8 or 8 × 16 | Same |
| Color | Four shade indices, mapped to display palettes | 15-bit RGB selection, 32,768 possible colors |

The CPU is not a complete Z80 and the MHz figure is not an instructions-per-second
rating. Double speed increases CPU capacity, not display resolution or refresh
rate. [Pan Docs specifications](https://github.com/gbdev/pandocs/blob/master/src/Specifications.md),
[CGB registers](https://github.com/gbdev/pandocs/blob/master/src/CGB_Registers.md).

### Graphics

The graphics hardware draws **8 × 8 tiles**, a scrolling background, a window
layer useful for HUDs/dialogue, and sprites. Two 32 × 32 tile maps each describe
a 256 × 256 area. Larger worlds are possible by updating map tiles as the camera
moves. GBC adds per-map-entry palette selection, tile flipping, bank selection,
and priority controls.
[Tile maps](https://github.com/gbdev/pandocs/blob/master/src/Tile_Maps.md).

GBC has eight background/window palettes of four colors and eight sprite
palettes with three visible colors plus transparency. That gives a conventional
budget of up to **56 simultaneously selected visible colors**, before advanced
mid-frame palette changes. A single sprite does not get all 32,768 colors.
[Palettes](https://github.com/gbdev/pandocs/blob/master/src/Palettes.md).

Large characters consume several hardware sprites. A 16-pixel-wide character
usually needs two sprites across, so a crowd can hit the ten-per-line limit
well before the total of forty. Busy scanlines require deliberate design.
VRAM/OAM access also has timing restrictions; update safely during permitted
periods, commonly VBlank, or use appropriate DMA routines.
[OAM](https://github.com/gbdev/pandocs/blob/master/src/OAM.md),
[VRAM/OAM access](https://github.com/gbdev/pandocs/blob/master/src/Accessing_VRAM_and_OAM.md).

Raster effects, parallax, animated palettes, pseudo-3D, and carefully optimized
software rendering are possible techniques. They spend CPU time and timing
complexity. There is no standard hardware facility for arbitrary sprite rotation,
scaling, textured polygons, or alpha blending. The console's frame-blend option
helps reproduce effects built from alternating frames; it does not add an alpha
channel to our ROM.
[Rendering](https://github.com/gbdev/pandocs/blob/master/src/Rendering.md),
[display settings](https://support.modretro.com/en_us/welcome-to-chromatic-system-settings-SkHcfgu0Zl).

### Sound

The standard audio unit offers two pulse channels, one programmable wave channel,
and one noise channel. Channels can be routed to left and right outputs. Music
and effects share this budget, so a sound effect may temporarily borrow a music
channel. This is enough for a sequencer, synth toy, rhythm game, and rich chiptunes.
Sample playback is possible through specialized techniques, with fidelity and
CPU tradeoffs. It is not a normal multitrack PCM player.
[Audio architecture](https://github.com/gbdev/pandocs/blob/master/src/Audio.md),
[audio registers](https://github.com/gbdev/pandocs/blob/master/src/Audio_Registers.md).

### Memory, ROM size, and saves

The CPU addresses 64 KiB at once. Larger ROMs and RAM use **bank switching**,
which maps selected chunks into that address space. Plan for banks early rather
than treating a cartridge as one flat memory allocation.
[Memory map](https://github.com/gbdev/pandocs/blob/master/src/Memory_Map.md).

| Cartridge feature | What it means |
| --- | --- |
| ROM-only | Small projects can fit in 32 KiB without a mapper |
| MBC5 | Standard addressing supports up to 8 MiB ROM and up to 128 KiB external RAM, depending on board configuration |
| MBC3 | Common route to cartridge RTC; standard configuration supports up to 2 MiB ROM and 32 KiB RAM |
| Persistent save storage | Requires suitable cartridge hardware and software support; capacity and persistence technology vary |
| RTC | Enables real-world time while off when the cartridge provides a functioning backed clock |
| Rumble, tilt, camera | Peripheral hardware lives in a special cartridge; it is not automatically built into the handheld |

An 8 MiB mapper limit does not mean the DevDay cartridge contains 8 MiB flash.
Likewise, adding an RTC flag to a ROM cannot create an RTC chip. Some modern
ModRetro carts use battery-free FRAM for saves, as seen in iFixit's Tetris
teardown, but that does not establish the specification of every cartridge.
[Mappers](https://github.com/gbdev/pandocs/blob/master/src/MBCs.md),
[MBC5](https://github.com/gbdev/pandocs/blob/master/src/MBC5.md),
[MBC3](https://github.com/gbdev/pandocs/blob/master/src/MBC3.md),
[Tetris cartridge teardown](https://www.ifixit.com/News/106916/modretro-chromatic-better-than-the-game-boy-color-it-emulates).

## Communication and computer integration

### Link cable

The official manual supports two-player links with Chromatic, GB, and GBC systems.
The connector/cable must suit both devices; do not assume a GBA multiplayer cable
is interchangeable. Our game must implement the protocol, synchronization,
disconnect handling, and timeouts.
[Link instructions](https://support.modretro.com/en_us/welcome-to-chromatic-link-cable-r1w_fe_Abg).

The GB serial interface exchanges one byte in each direction per transfer. Its
standard internal clock is 8,192 bits/second, roughly 1 KiB/second before gaps and
protocol overhead. GBC offers faster clock modes, up to 524,288 bits/second in
double-speed mode. Those are protocol ceilings, not measured Chromatic throughput.
Start with the slow mode for broad compatibility. A custom external adapter could
bridge the link to a computer or microcontroller, but needs suitable electrical
interfacing and its own software.
[Serial reference](https://github.com/gbdev/pandocs/blob/master/src/Serial_Data_Transfer_%28Link_Cable%29.md).

### Infrared

IR can exchange small messages with another Chromatic or GBC. ModRetro recommends
roughly 4 to 5 cm separation, a clear path, and no direct sunlight. GBC's IR
register exposes transmitter/receiver control; it does not provide a packet or
network stack. Trading collectibles or exchanging a challenge seed is a better
first use than continuous action multiplayer. IR and the link cable are separate
transports, and a game's support for one does not imply support for the other.
[IR setup](https://support.modretro.com/en_us/welcome-to-chromatic-ir-port-BydPMlO0l),
[IR programming](https://github.com/gbdev/pandocs/blob/master/src/IR.md).

### USB capture, tools, and live demos

| USB use | Status and limits |
| --- | --- |
| Capture gameplay video | Official, appears as a webcam/video capture source |
| Capture gameplay audio | Official since firmware 4.0; add the Chromatic audio input separately in OBS |
| Capture two consoles | Official guide uses distinct player numbers; USB bandwidth/topology can matter |
| Firmware update | Official MR Updater |
| Cartridge patch/save management | Official Cart Clinic for supported ModRetro cartridges |
| Inspect/write homebrew carts | Official CLI advertises these commands; compatible cart and developer access may be required |
| Stream a host-emulated ROM to the handheld | Official CLI `live-demo`; still needs testing on our unit |
| Direct HDMI/DisplayPort monitor output | Not documented; USB capture is not evidence of video alternate mode |
| Generic USB controller, keyboard, MIDI, or game networking | No established stock API identified in the reviewed documentation |

Use OBS point filtering and preserve the 10:9 aspect ratio. The published v4.2
FPGA code advertises a 160 × 144, 60 fps UVC stream and stereo 16-bit USB audio.
Firmware 4.5 says streaming resolution improved but does not publish the new
mode details in its changelog. Inspect the actual device before hard-coding a
capture format. Capture latency also depends on the computer and application.
[Capture guide](https://modretro.com/blogs/blog/how-to-record-and-stream-gameplay-on-chromatic),
[USB source](https://github.com/ModRetro/oss-chromatic-console-fpga/tree/210176f9b7a4e6a8c25190e4a1620d2c1c97ba74/esp32t/src/rtl/USB/USBUVCUART),
[firmware changelog](https://modretro.com/pages/chromatic-firmware-changelog).

`live-demo` and cartridge execution are different test environments. The former
runs emulation on the computer; the latter exercises the Chromatic's FPGA core
and the real cartridge. Passing a live demo is not a substitute for standalone
testing. See the [development guide](development.md) for verified command syntax.

## System settings and firmware

The manual documents brightness, silent mode, frame blending, stream color
correction, classic/smooth screen transitions, low-battery icon behavior,
monochrome-game palettes, optional suppression of diagonal inputs, firmware
details, serial number, and capture player number. Opening the system menu
**does not automatically pause the game**.
[Settings](https://support.modretro.com/en_us/welcome-to-chromatic-system-settings-SkHcfgu0Zl).

Useful shortcuts:

| Shortcut | Action |
| --- | --- |
| Menu + Right / Left | Raise / lower brightness |
| Menu + Up / Down | Change GB palette or GBC color temperature, per current manual |
| A + B + Start + Select | Game reset, if the game implements it |
| Menu + A + B + Start + Select | Reset emulation core and game |

Boot-time palette shortcuts also exist. Saved palette preferences can override
them. Custom GB palettes and GBC color-temperature controls are distinct features.
[Hotkeys, updated September 24, 2026](https://support.modretro.com/en_us/welcome-to-chromatic-hotkeys-ByMjGgdAZe).

**Version snapshot:** the official release feed lists **4.5 on September 28,
2026**, including new-device support, streaming-resolution improvements, and
palette/color-temperature hotkeys. Public source repositories still identify
**4.2**, with FPGA **18.8** and MCU **0.13.4**. This mismatch matters when
evaluating a newly issued device or building replacement firmware.
[Release feed](https://s3.us-east-1.amazonaws.com/updates.modretro.com/firmware/chromatic/chromatic_changelog.yaml),
[MCU release](https://github.com/ModRetro/oss-chromatic-console-mcu/releases/tag/v4.2).

Cart Clinic can patch supported commercial carts and back up, restore, or erase
their saves. Some updates invalidate old saves. This is separate from installing
an arbitrary homebrew ROM; do not treat the updater's patch button as a generic
ROM loader. [MR Updater and Cart Clinic](https://support.modretro.com/en_us/chromatic-firmware-updater-ryhoYnzCx).

## Deeper modifications and experimental capabilities

### Open firmware

ModRetro publishes FPGA HDL and ESP32 MCU firmware. The inspected FPGA project
targets a Gowin GW5A-25A-family device and derives its game core from the MiSTer
Game Boy project. The ESP32 handles system services and communicates with the
FPGA. Ordinary game ROMs do not automatically inherit those system interfaces.
[FPGA project](https://github.com/ModRetro/oss-chromatic-console-fpga/blob/210176f9b7a4e6a8c25190e4a1620d2c1c97ba74/esp32t/evt1_x2.gprj),
[MCU board interface](https://github.com/ModRetro/oss-chromatic-console-mcu/blob/6c94613da72420f6a364e0024681401bd69184ba/main/board.h).

The documented FPGA build requires **Gowin FPGA Designer 1.9.9.03** and recursive
submodules. The MCU guide uses **ESP-IDF 5.3**. FPGA loading can use the Gowin
programmer or a build of `openFPGALoader` with GWU2X support; MCU development has
a USB serial route. These are substantially different toolchains from compiling
a game. Firmware experimentation should wait until the DevDay hardware revision
and a compatible restore image are known.
[FPGA build instructions](https://github.com/ModRetro/oss-chromatic-console-fpga#readme),
[MCU build instructions](https://github.com/ModRetro/oss-chromatic-console-mcu#readme).

### Community projects

| Project | Author-documented capability | Boundary |
| --- | --- | --- |
| [ChroMagic](https://github.com/cursedtoast2/ChroMagic) | Cartridge/save backups and on-device SD-backed playback | Custom firmware; requires an installed SD card for on-device storage/playback |
| [ChroMagician](https://github.com/cursedtoast2/ChroMagician) | USB cartridge tools, supported homebrew writing, SD file management, firmware installation | Companion app; published installer guidance lists Windows and Linux |
| [Fred Emmott's FlashGBX fork](https://github.com/fredemmott/FlashGBX/releases/tag/v5.1%2Bfredemmott.3) | Chromatic-backed cartridge/save tools, including Mac builds | Community FPGA/tooling path; cartridge compatibility needs checking |
| [PrismGB](https://github.com/josstei/prismgb-app) | Desktop viewing, screenshots, recording, rendering effects | Effects run on the computer, not inside a standard game ROM |

These are author claims inspected during research, not hardware validation.
SD installation/access and board compatibility must be established for our unit.
There is no basis here to assume every Chromatic ships with a usable external SD
slot or that stock firmware includes a ROM browser.

### Wireless, other cores, and hardware accessories

The ESP32 and open firmware make wireless experimentation interesting, but we
have not established a supported stock Wi-Fi, Bluetooth audio, internet, or
game-networking API. A chip's potential features are not a shipping product
feature. Radio behavior, antenna implementation, firmware, and the path between
the ROM and MCU all need verification.
[MCU source](https://github.com/ModRetro/oss-chromatic-console-mcu),
[physical teardown](https://www.ifixit.com/News/106916/modretro-chromatic-better-than-the-game-boy-color-it-emulates).

New FPGA cores, custom peripherals, and altered system services are engineering
projects, not established stock capabilities. GB/GBC compatibility does not imply
GBA, NES, SNES, N64, or arbitrary desktop software support. Special cartridges
may add cameras, tilt, rumble, or other interfaces; test the exact accessory.
The official changelog specifically added Kirby Tilt 'n' Tumble support in 3.0.
[Changelog](https://modretro.com/pages/chromatic-firmware-changelog).

ModRetro's M64 page describes future Transfer Pak support through Chromatic.
Treat that as a roadmap item until a shipping implementation is confirmed, not
something our first game can require.
[M64 product FAQ](https://modretro.com/products/m64).

## Practical limits to remember

- A standard ROM has no documented direct web/API access. Offline generated
  dialogue, levels, and art can be compiled into it; live cloud interaction
  needs an external bridge or additional system work.
- No stock save-state, rewind, suspend/resume, or fast-forward facility was
  established in the reviewed manual. Implement in-game saves and pause.
- No built-in touchscreen, microphone, camera, analog stick, or motion API was
  established. A USB audio source named “Microphone” represents game audio.
- Bigger screens on a computer and postprocessing do not increase the game's
  native resolution or sprite budget.
- A PC live demo still needs the PC. For a portable result, finish with a
  compatible cartridge and a standalone hardware test.

These boundaries describe the reviewed stock interfaces, not a proof that every
future firmware modification is impossible. See [research gaps](research-sources.md)
for what remains to verify.
