# Building for our Chromatic

Checked September 29, 2026. The device has not yet been collected or tested.
The first goal should be one small **GBC game that runs from a cartridge**.

## Choose a toolchain

| Tool | Best fit | Tradeoff |
| --- | --- | --- |
| [GBDK-2020](https://github.com/gbdk-2020/gbdk-2020) | C code, custom game mechanics, reusable engine code | We manage graphics budgets, memory, and banking |
| [GB Studio](https://www.gbstudio.dev/docs/) | Fast visual prototyping, adventures, platformers, scripted scenes | Engine conventions and editor workflow shape the game |
| [RGBDS](https://github.com/gbdev/rgbds) | Assembly, precise timing, unusual effects and peripherals | More low-level code and debugging |

Recommendation: **GBDK-2020 for this code repository**, unless the DevDay workshop
provides a starter kit worth keeping. GB Studio is an excellent alternative for
a content-led first game. RGBDS is useful when timing or CPU limits justify it.
These are choices based on our likely workflow, not requirements imposed by
Chromatic.

Versions observed: [GBDK 4.5.0](https://github.com/gbdk-2020/gbdk-2020/releases/tag/4.5.0),
[GB Studio 4.3.2](https://github.com/chrismaltby/gb-studio/releases/tag/v4.3.2),
[RGBDS 1.0.4](https://github.com/gbdev/rgbds/releases/tag/v1.0.4).
Pin whichever we adopt in the game's README/build setup.

Use [SameBoy](https://sameboy.github.io/) for quick local tests and debugging.
For music, [hUGETracker](https://github.com/SuperDisk/hUGETracker) and its linked
hUGEDriver provide composition and in-game playback tools. For graphics, keep
editable source art and convert to the target's tiles/palettes with the chosen
toolchain. GB Studio exports a ROM or a separate web build; the web build is an
emulator wrapper, not a browser running on the handheld.
[GB Studio export guide](https://www.gbstudio.dev/docs/build/).

## Recommended development loop

1. Create `games/<game-name>/` with source, editable assets, and a README.
2. Build a `.gb` or `.gbc` ROM. Check header, mapper, size, and color-mode settings.
   The extension alone does not determine hardware compatibility.
3. Run it in an emulator. Fix input, memory, rendering, and save problems there.
4. If developer access permits, use the official live demo to evaluate the
   handheld display and interaction workflow.
5. Write the ROM to a compatible development cart and test with USB unplugged.
6. Test saves across power cycles and test link/IR on actual devices if used.

Example project layout, to adopt when creating the first game:

```text
games/<game-name>/
  README.md
  Makefile
  src/
  assets/
  build/       # generated and ignored by this project's .gitignore
```

Build/test commands should be separate from commands that write cartridges.
Keep each game's build reproducible without requiring a connected console.

## Official Chromatic CLI

Package: [`@modretro/chromatic-cli`](https://www.npmjs.com/package/@modretro/chromatic-cli).
Version **1.2.1** was downloaded into a temporary research directory and its
`--help` output inspected locally. It was not globally installed. No device
commands, activation, cartridge writes, or firmware changes were performed.

The package provides binaries for Apple Silicon/Intel macOS, ARM64/x64 Windows,
and ARM64/x64 glibc Linux. Its README says macOS needs no additional driver
installation. Windows/Linux setup differs from macOS, and the desktop updater's
platform matrix is not identical to the CLI's.

Installation, when setting up the workstation:

```sh
npm install --global @modretro/chromatic-cli@1.2.1
chromatic-cli --version
chromatic-cli --help
```

Verified command surface:

| Command | Purpose |
| --- | --- |
| `install-drivers` | Configure required USB drivers/permissions on supported systems |
| `list-devices` | List attached Chromatics and USB functions |
| `detect-cart --all` | Inspect flash-chip identity and cartridge-header fields |
| `activate <CODE>` | Enable features for this computer using a code from ModRetro |
| `live-demo <ROM>` | Host-emulate GB/GBC ROM and stream video/audio to Chromatic |
| `write-homebrew <ROM>` | Write a ROM at the beginning of cartridge flash |
| `reset-device` | Reload the configured FPGA image |

Global output supports `--format human`, `json`, or `jsonl`; the last is useful
for live-demo lifecycle events. Commands also expose `--yes`, which bypasses
their prompts. Retain prompts for initial hardware setup. Relevant commands
offer `--player` to choose among multiple consoles.

Source for command details: the shipped 1.2.1 executable's help, reached through
the [official package](https://www.npmjs.com/package/@modretro/chromatic-cli).
The package README documents fewer commands than the binary.

### Identify the connected setup

After collecting the device, insert/remove cartridges only while powered off.
Connect a USB data cable, power on, then inspect:

```sh
chromatic-cli list-devices --format json
chromatic-cli detect-cart --all --format json
```

Keep the cart's mapper, ROM capacity, RAM/save capacity, and any RTC/rumble
details with the project notes. Header claims should be checked against the
actual cartridge specification. Record the console revision and firmware shown
in its system menu. Do not publish an activation code or a device serial number.

### Live demo

For a ROM built at the example path below:

```sh
chromatic-cli live-demo games/<game-name>/build/game.gbc --duration 60 --no-save
```

Replace `<game-name>` with the real folder before running. For persistent demo
saves, omit `--no-save` and use `--save-dir <directory>`. The command also supports
`--expect-sha256 <digest>` to ensure it receives the intended ROM.

Confirmed from help: host emulation, video/audio streaming, duration, and save
directory options. **Not yet confirmed on our hardware:** activation scope,
minimum firmware, whether a cartridge must be present, button-input behavior,
latency, and compatibility with the DevDay edition. No generic desktop mirroring
or arbitrary video-input API was established by this command.

### Cartridge deployment

Once the cartridge is identified as a development cart and its contents may be
overwritten, the documented syntax is:

```sh
chromatic-cli write-homebrew games/<game-name>/build/game.gbc
```

This writes cartridge flash. Preserve any needed existing game/save contents
first with a supported backup tool. Confirm the mapper and available storage
match the ROM. The CLI's top-level help does not advertise a backup command.
Do not assume `write-homebrew` works on every third-party flash cartridge.

Alternatives include a verified compatible multicart or a dedicated cart writer
with a matching rewritable board. Choose exact hardware only after we know what
DevDay supplies. Community options are listed in the
[capabilities guide](chromatic-capabilities.md#community-projects).

### Developer-access terms

The current updater terms say developer features may require activation and are
for personal, non-commercial development/testing unless ModRetro agrees
otherwise in writing. Activation credentials are personal. This is a restriction
on that developer service/tool path, not evidence that all GB/GBC development is
non-commercial. Check the terms applicable to the chosen path before planning a
commercial release. ModRetro separately invites developers to discuss publishing.
[Updater terms, §6, September 24, 2026](https://modretro.com/pages/eula),
[publishing invitation](https://modretro.com/blogs/blog/create-for-chromatic).

## Capture a build on the Mac

The official workflow is USB video plus USB audio in OBS. Add a video-capture
source for Chromatic and a separate audio-input source for its game audio.
Preserve the image's 10:9 aspect ratio and use point filtering. Multiple devices
should have distinct capture player numbers. Confirm audio monitoring separately
from whether sound is being recorded.
[Official capture instructions](https://modretro.com/blogs/blog/how-to-record-and-stream-gameplay-on-chromatic).

For the new 4.5 firmware, inspect available capture modes rather than assuming
the older source code's 160 × 144 USB descriptor remains unchanged. The physical
game display is still the 160 × 144 target documented in the product materials.

## Questions to take to the DevDay session

These are the unresolved details that will change our implementation choices:

1. Which Chromatic revision is this? Is there edition-specific firmware or a
   required minimum version for the workshop tools?
2. Is a rewritable cartridge included? What are its mapper, flash capacity,
   save capacity/type, RTC, and supported write-cycle specifications?
3. Is a developer activation code included, and which CLI features does it
   enable? Is there a public workshop repository or setup guide?
4. Does `live-demo` read the physical controls, require a cart, and work with
   the supplied firmware? What does it change temporarily on the device?
5. What is the supported backup/restore workflow before overwriting a cart?
6. Are there documented game-to-host or game-to-MCU APIs beyond standard link
   and IR? Is wireless development officially supported on this edition?
7. Is a public 4.5 source release and matching recovery image available? Are
   older custom firmware images compatible with the new hardware?

## First hardware validation

When the unit arrives, test a small diagnostic ROM before a large game:

- All game buttons, held inputs, diagonals, and pause behavior.
- A labeled color chart and tile/sprite test, including crowded scanlines.
- Pulse, wave, and noise sounds, plus left/right headphone output and USB audio.
- Save, power off, reboot, and load on the chosen cartridge.
- A normal-speed link handshake and an IR message if we plan to use them.
- Both host live-demo and cartridge boot; confirm the final game runs untethered.

These are planned checks. No hardware check has passed yet.
