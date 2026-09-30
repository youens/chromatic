# Collection integration review

Review of the three-game expansion and cartridge launcher integration.

The three new games, scanline interrupt lifecycle, banked build, save checksum,
launcher navigation, and browser Select handling were inspected. No blocking
issue remains in the integrated build after the changes below.

- Added Dot Swarm as the tenth game without reverting its tuned pace or art.
- Moved Hello Dot and launcher-art banks to avoid the added game's banks.
- Bumped the save format version for the changed records array layout.
- Preserved Chromatic Fun by Youens branding and Start+Select home guidance.
- Replaced count-specific website branding and pixel-poster thumbnails with
  illustrated covers and an evergreen share image.
- Kept Dot Swarm's separate timing tests; its deliberate update cadence differs
  from the eight games tested at 30 Hz.
- Initialized the emulator save buffer before tests so an early assertion does
  not mask the original failure during cleanup.

Validation: all ten games launch and return twice; saved scores/play counts and
last selection survive a simulated power cycle; erase and checksums pass.
Bots complete Stormkite, all nine Comet Links holes with exact physics parity,
and all five Prism Well stages. Original games' completion tests also pass.
The integrated ROM is 512 KiB MBC5 with 8 KiB save RAM. The build's WRAM guard
passes with 871 bytes free below the fixed screen buffers, which limits future
additions unless game RAM is overlaid or reduced.

Physical boot, gameplay and battery-save persistence require observation on
hardware; successful flashing alone does not establish them.
