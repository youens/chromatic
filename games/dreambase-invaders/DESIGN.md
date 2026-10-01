# Dreambase Invaders design

**Genre:** inverted arcade shooter · **Length:** three to five minutes a run ·
**Core question:** chase the logo about to escape, or keep the streak alive?

A Game Boy Color edition of the Dreambase Invaders web game (React and
canvas). The rules, level order and tuning follow its `team/GAMEPLAY.md`.
The web version's 640 × 800 portrait field becomes a 160 × 96 pixel sky above
the lunar base, so horizontal distances scale by a quarter and the game runs
on frames at 60 Hz instead of milliseconds.

## Pillars

1. **Eating feels good.** Catching is generous (a 24 × 24 box around the
   analyst), every catch gulps with a mouth-open frame, a white flash, a
   spark burst, a floating score and a two-note wakka whose pitch climbs with
   the streak multiplier.
2. **Brand first.** The Dreambase mark opens the cartridge, glows on the
   lunar base and ends the game as the final form. Each level is a partner
   company: its logo is the projectile, its colour the turrets, the meter
   and the HUD, and you become its invader.
3. **Readable at 160 × 144.** Logos are 16 × 16 sprites on a dark sky. A
   turret flashes white for a quarter of a second before it fires.

## Controls

| Input | Action |
| --- | --- |
| D-pad | Move in eight directions: 1.5 px per frame, 1.06 px on each axis diagonally |
| A or B | Dash: 4 px per frame for 8 frames in the facing direction, 40-frame recharge |
| Start | Pause (the screen dims to half brightness) |
| Select | Sound |

The analyst stays in the sky: x 10-150, y 10-76 (centre).

## The eight stacks

Turrets sit at x 24, 48, 80 (the dome hatch), 112 and 136. Three turrets use
the middle three slots, four use the outer four, and five use them all.

| Level | Company | Turrets | Fire interval | Logo speed | Patterns (straight / aimed / spread / wave) | Catches (Easy) | Points | Drain per miss |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Supabase | 3 | 78 frames | 26/64 px | 100 / 0 / 0 / 0 | 10 | 10 | 8 |
| 2 | Stripe | 3 | 69 | 30 | 80 / 20 / 0 / 0 | 12 | 15 | 9 |
| 3 | PostHog | 4 | 60 | 34 | 70 / 0 / 30 / 0 | 14 | 20 | 10 |
| 4 | Vercel | 4 | 54 | 38 | 50 / 20 / 0 / 30 | 16 | 25 | 11 |
| 5 | GitHub | 4 | 48 | 44 | 40 / 40 / 20 / 0 | 18 | 30 | 12 |
| 6 | ClickHouse | 5 | 43 | 50 | 0 / 20 / 40 / 40 | 20 | 40 | 12 |
| 7 | Linear | 5 | 38 | 57 | 0 / 50 / 20 / 30 | 22 | 50 | 13 |
| 8 | Dreambase | 5 | 33 | 65 | 25 / 25 / 25 / 25 | 25 | 60 | 14 |

Speeds are in 1/64 pixel per frame. Logos rise 120 px from the turrets to
the top of the screen: about 4.9 seconds on level 1 and 2 seconds on level 8
at Easy speeds. Fire intervals get ±10% jitter, and
turrets fire in a shuffled round robin. The life bar holds 96; each catch
heals 2.

| Difficulty | Catches per level | Fire interval | Logo speed |
| --- | --- | --- | --- |
| Easy | as listed | as listed | as listed |
| Normal (default) | × 1.5 | − 10% | + 10% |
| Hard | × 2 | − 20% | + 20% |

### Patterns

- **Straight:** straight up.
- **Aimed:** towards the analyst's position when fired, at most about 35°
  from vertical. A turret never fires two aimed shots in a row.
- **Spread:** three logos at once, the outer two drifting a quarter of the
  speed sideways. Each is caught or missed on its own.
- **Wave:** straight up while swaying ±12 px over about a second.

## Economy

| Event | Points |
| --- | --- |
| Catch | level points × streak multiplier (x2 from 5, x4 from 10, x8 from 15) |
| Logo still in the air at level-up | 5 |
| Perfect level (no misses) | 100 × level |
| 5,000 points | one extra life (once) |

Scores are 16-bit and stop at 65,535.

## Level-up

When the meter fills, every logo in the air pops into gold sparks for 5
points each and play freezes for 200 frames:

| Frame | Event |
| --- | --- |
| 0-24 | The meter flashes |
| 12 | Fanfare |
| 24-96 | The analyst flickers between its form, white and the new form, with orbiting sparks; the stars rush past |
| 60 | The new company's colours take over the turrets, HUD and logos |
| 96 | A spark burst |
| 100-170 | A banner: LEVEL n, the company in double-height letters between two of its logos, and any perfect bonus |
| 200 | Play resumes; the first shot waits 48 frames |

Level 1 opens with the same banner. After level 8 the analyst becomes the
Dreambase final form and the credits roll: every company's invader beside its
name and role, your score, and an invitation to dreambase.com.

## Feel

| Effect | Detail |
| --- | --- |
| Catch | Mouth open for 8 frames, white flash for 3, four sparks, a floating score or multiplier |
| Miss | The sky flashes red for 8 frames, a 1 px shake, a descending tone |
| Life lost | The screen flashes white, logos clear, a 20-frame shake, 90 frames of invulnerability while the turrets hold fire |
| Heartbeat | A four-note bass on the wave channel, one note per fire interval, up to twice as fast as the meter fills |

## Hardware

| Feature | Technique |
| --- | --- |
| Title copper gradient and ripple | HBlank STAT interrupts for the 24 lines of INVADERS; each sets the next line's SCX and background palette colour |
| Wordmark shine | Two LY=LYC interrupts per frame switch palette colours on and off |
| Starfield | 32 × 32 background map scrolled with SCY |
| Lunar base and HUD | Window layer from line 96 |
| Logos rising out of turrets | Background-priority attribute on turret and dome tiles |
| Map updates | 32 × 20 shadow maps in WRAM, dirty rows copied by general-purpose DMA in VBlank |
| Palette effects | All palette writes queued and uploaded by the VBlank handler |
| Title formation | Invader tiles recoloured at load so two invaders share one sprite palette |
| Sound | Square lead, wave-channel bass, noise drums; channel 1 effects |

Re-arming LYC for the very next line races GBDK's interrupt epilogue, which
acknowledges the STAT flag as it returns, so the per-line effect uses the
HBlank interrupt instead.
