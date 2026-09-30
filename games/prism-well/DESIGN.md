# Prism Well design

**Genre:** falling crystal puzzle · **Length:** five wells, 8–15 minutes ·
**Core question:** clear now, or build toward a chain?

Crystals of coloured light fall into an old mine well. Each piece is a
vertical trio that you move and cycle. Line up three or more of a colour in
any direction and they shatter. Crystals above fall and may shatter again.
Deeper wells are floored with ancient stone that only breaks when light
shatters right beside it.

## Pillars

1. **Colour is never the only signal.** Each colour has its own silhouette:
   ruby diamond, amber orb, jade triangle, sapphire square, amethyst cross.
2. **Every move shows its result.** A dotted ghost marks where the trio will
   land, and the next piece is always visible.
3. **Short-term and long-term goals.** A greedy match breaks one stone; a
   planned cascade breaks several.

## Controls

| Input | Action |
| --- | --- |
| ← → | Move; auto-repeat after 6 updates |
| A | Cycle colours downward (the bottom crystal moves to the top) |
| B | Cycle colours upward |
| ↓ | Soft drop, one row per update (+1 point per row) |
| ↑ | Hard drop and lock (+2 points per row) |
| Start | Pause |
| Select | Sound |

A piece locks 12 updates after it comes to rest, or 2 updates if Down is
held, so there is time to slide it into a gap.

## Wells

| Well | Goal | Colours | Fall interval | Stone floor (bottom row first) |
| --- | --- | --- | --- | --- |
| 1 | Shatter 30 lights | 4 | 18 updates | none |
| 2 | Break every stone | 4 | 16 | `S.S.S.S` |
| 3 | Break every stone | 5 | 14 | `SS.S.SS`, `.S...S.` |
| 4 | Break every stone | 5 | 12 | `SSS.SSS`, `S.S.S.S` |
| 5 | Break every stone | 5 | 10 | `SSS.SSS`, `SS...SS`, `.S...S.` |

The well is 7 columns by 15 rows. A new trio enters at the top of the centre
column. If that cell is filled, or a trio locks with any crystal above the
rim, the run ends.

## Rules

* **Matching:** runs of three or more in a row, a column or either diagonal.
  Every run is found in one scan; runs that cross share crystals.
* **Stones** never match. A stone breaks when a shattering crystal is
  orthogonally next to it. Stones fall like everything else.
* **Cascades:** after a shatter, every unsupported cell drops one row per
  update until the well settles. The well is then scanned again, and each
  further shatter raises the chain count.
* **Prism:** every twentieth piece is a prism trio. The panel counts down to
  it. When a prism lands on a crystal, every crystal of that colour
  shatters. On stone it simply shatters, breaking stones beside it; on the
  empty floor it shatters for 300 points.

## Scoring

| Event | Points |
| --- | --- |
| Each shattered light | 10 × chain × well |
| Each broken stone | 50 × well |
| Well cleared | 1,000 × well |
| Prism into the floor | 300 |

## Art and sound

Gems are 7 × 7 glyphs in 8 × 8 tiles, leaving a one-pixel gutter between
neighbours. Each colour has its own palette: dark rim, body and highlight.
The well walls are dim mine brick; the side panels show the next piece, the
goal, and the prism countdown. Cleared crystals flash a white starburst for
12 updates before they vanish.

This is the first game in the collection to use the wave channel. A soft
triangle waveform is loaded into wave RAM. Each shatter in a chain plays the
next note of a pentatonic scale, over a pulse-channel tone that rises with
the chain.

## Budgets

| Resource | Use |
| --- | --- |
| ROM | 32 KiB cartridge, 13.1 KiB used |
| Work RAM | 280 bytes; the board and match marks are 105 bytes each |
| Background tiles | 15 custom |
| Sprites | the falling trio (3) |
| Frame time | Scans step through indices instead of multiplying. The well is redrawn only when it changes, and never on the same update as a scan or shatter |

## Verification

The test bot tries every column and colour order with a Python copy of the
rules. It scores stones broken, lights shattered, chains, stack height and
same-colour neighbours, then drives the piece there with the joypad. It
must clear all five wells while averaging at least 98% of the 30 Hz target.
The recorded run placed 95 pieces.
