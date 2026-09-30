# Research record

Checked **September 29, 2026**. This records publicly accessible documentation,
author-maintained code, and local inspection of the official CLI package.
It does not certify exhaustive compatibility or replace testing the actual unit.

## Evidence levels

- **Official documented:** ModRetro manual, product pages, release feed, and
  distributed CLI help.
- **Standard platform behavior:** Pan Docs and maintained GB/GBC toolchain docs.
  These establish the programming model, not that every accessory is tested.
- **Source-inspected:** public firmware code at the revisions below. It may
  lag shipping binaries.
- **Community documented:** the project's own README/release notes. Not tested
  here and not an official support commitment.
- **Proposal or unknown:** explicitly labeled design inference or research gap.

## Version and provenance snapshot

| Item | Observed version/revision | Evidence |
| --- | --- | --- |
| Current firmware release feed | 4.5, dated 2026-09-28 | [Official YAML](https://s3.us-east-1.amazonaws.com/updates.modretro.com/firmware/chromatic/chromatic_changelog.yaml), linked by the [changelog page](https://modretro.com/pages/chromatic-firmware-changelog) |
| Public FPGA repository | `210176f9b7a4e6a8c25190e4a1620d2c1c97ba74`, FPGA 18.8 | [Pinned source](https://github.com/ModRetro/oss-chromatic-console-fpga/tree/210176f9b7a4e6a8c25190e4a1620d2c1c97ba74) |
| Public MCU repository | `6c94613da72420f6a364e0024681401bd69184ba`, product 4.2 / MCU 0.13.4 | [Pinned configuration](https://github.com/ModRetro/oss-chromatic-console-mcu/blob/6c94613da72420f6a364e0024681401bd69184ba/sdkconfig.defaults) |
| Chromatic CLI | 1.2.1, Darwin ARM64 binary | [Official npm package](https://www.npmjs.com/package/@modretro/chromatic-cli) |
| Pan Docs checkout | `0edc96012625b16d275674fea4f36aa6b0bb1d45` | [Pinned source](https://github.com/gbdev/pandocs/tree/0edc96012625b16d275674fea4f36aa6b0bb1d45) |
| GBDK-2020 | 4.5.0 | [Release](https://github.com/gbdk-2020/gbdk-2020/releases/tag/4.5.0) |
| GB Studio | 4.3.2 | [Release](https://github.com/chrismaltby/gb-studio/releases/tag/v4.3.2) |
| RGBDS | 1.0.4 | [Release](https://github.com/gbdev/rgbds/releases/tag/v1.0.4) |
| Fred Emmott's FlashGBX | 5.1+fredemmott.3 | [Release](https://github.com/fredemmott/FlashGBX/releases/tag/v5.1%2Bfredemmott.3) |

The CLI package wrapper's npm SHA-1 was
`e45a92277443d0e15fe59415c4c4903c1d50fc78` and the Darwin ARM64 package's was
`4efcefa0b559c411d4236985adda3c005c91a978`. These identify the inspected npm
artifacts; they are not a separate security endorsement.

The locally executed commands were only `--version`, top-level `--help`, and
subcommand `--help` for `live-demo`, `write-homebrew`, `detect-cart`, `activate`,
`list-devices`, and `reset-device`. No device was connected for this research.
The package README omits some commands that the binary advertises.

## Main references

| Reference | Used for |
| --- | --- |
| [Chromatic manual index](https://support.modretro.com/en_us/chromatic-manual-S1cy8e_0Zx) | Hardware, power, IR, link, settings, hotkeys |
| [Retail product](https://modretro.com/products/chromatic-tetris-bundle) | Display, ports, compatibility, materials |
| [Capture guide](https://modretro.com/blogs/blog/how-to-record-and-stream-gameplay-on-chromatic) | OBS, USB audio/video, multiple consoles |
| [MR Updater](https://support.modretro.com/en_us/chromatic-firmware-updater-ryhoYnzCx) | Firmware updates and Cart Clinic scope |
| [Updater terms](https://modretro.com/pages/eula) | Developer activation and commercial-use boundary |
| [Create for Chromatic](https://modretro.com/blogs/blog/create-for-chromatic) | Official developer/publishing invitation |
| [Pan Docs](https://github.com/gbdev/pandocs) | CPU, memory, graphics, audio, serial, IR, cartridge mappers |
| [GBDK](https://github.com/gbdk-2020/gbdk-2020) | C toolchain and examples |
| [GB Studio](https://www.gbstudio.dev/docs/) | Visual game creation and export |
| [RGBDS](https://github.com/gbdev/rgbds) | Assembly tools and graphic conversion |
| [SameBoy](https://sameboy.github.io/) | Mac-compatible emulator/debugger |
| [hUGETracker](https://github.com/SuperDisk/hUGETracker) | Music authoring and playback ecosystem |
| [ChroMagic](https://github.com/cursedtoast2/ChroMagic) / [ChroMagician](https://github.com/cursedtoast2/ChroMagician) | Community SD playback and cartridge tools |
| [PrismGB](https://github.com/josstei/prismgb-app) | Community desktop capture |
| [iFixit teardown](https://www.ifixit.com/News/106916/modretro-chromatic-better-than-the-game-boy-color-it-emulates) | Physical observations and Tetris cartridge FRAM |

Direct links accompany the relevant claims in the guides. Some ModRetro support
pages are rendered dynamically; their advertised `.md` alternate URLs were used
to read the article body. The firmware changelog is also dynamic, so its official
YAML data feed was inspected rather than relying on the empty page shell.

## What remains unresolved

| Question | Why it matters | How to resolve |
| --- | --- | --- |
| Exact DevDay edition and revision | New firmware mentions new-device support | Inspect supplied documentation and console |
| Included cartridge specification | Determines ROM size, mapper, saves, RTC, and flashing route | Workshop spec plus `detect-cart --all` |
| Activation access and minimum firmware | Determines availability of CLI developer functions | Workshop guide and ModRetro entitlement |
| Live-demo input, latency, cartridge requirement, and temporary device changes | Determines quality and safety of the iteration workflow | Documented workshop behavior plus a small hardware test |
| Firmware 4.5 source and recovery compatibility | Public 4.2 code may be unsuitable for newer hardware | Official release/source confirmation |
| Actual 4.5 USB capture modes | Changelog announces resolution improvements without mode details | Query capture formats on the connected device |
| Exact flash-cart/accessory compatibility | Product-level compatibility is not a per-board test matrix | Verify model, board revision, and firmware, then test |
| Internal SD provision | Community firmware requires installed storage | Edition-specific board documentation; inspect only if needed |
| Supported wireless/game-to-host interfaces | No stable stock API established by reviewed sources | Ask ModRetro and inspect published interface documentation |
| Repair CAD and current replacement-part availability | Older teardown links now redirect to support | Obtain current files from ModRetro before designing replacement parts |

Community posts were useful discovery leads, but unsupported claims such as
universal cartridge compatibility, ready-to-use wireless networking, arbitrary
alternate cores, or confirmed DevDay bundle contents were not promoted to facts.
