# Comet Links design

**Genre:** gravity mini golf · **Length:** nine holes, 5–10 minutes ·
**Core question:** aim straight, or let a planet bend the shot?

Nine holes across an asteroid belt. You putt a tiny comet into wormhole cups
while planets pull every shot off its line. Moons make slingshots, solar wind
makes rivers, bumpers ricochet, and a black hole will swallow a careless putt.

## Pillars

1. **Physics you can learn.** There is no randomness. The same aim and power
   always produce the same flight, on the cartridge and in the Python model.
2. **Every planet pulls.** Gravity falls off as 1/distance, so a planet's
   influence reaches across the whole hole. Reading those curves is the skill.
3. **Foresight is a resource.** The scope shows the true path, gravity
   included, but you only have a few.

## Controls

| Input | Action |
| --- | --- |
| ← → | Aim; 64 directions, repeating when held |
| ↑ ↓ | Power, 1–16, shown as eight half-step pips |
| A | Putt |
| B | Scope: trace the real path for this stroke (uses a charge) |
| Start | Pause |
| Select | Sound |

Without the scope, four guide dots show the direction and a rough length.
The scope traces up to 160 physics steps and drops a dot every tenth. It adds
five steps per update, so the path draws itself in about a second while you
keep aiming.

## Course

| Hole | Par | Idea |
| --- | --- | --- |
| 1 First Light | 2 | A small asteroid blocks the direct line; nebula patches teach heavy drag |
| 2 Moon Bend | 3 | A planet sits between tee and cup; curve around it |
| 3 Solar Wind | 3 | An updraft river lifts the ball toward a walled cup |
| 4 Bumper Alley | 3 | Pass a gap in the wall, then use or avoid bumpers |
| 5 Twin Moons | 3 | Two moons guard a cup in a narrow pocket |
| 6 Event Horizon | 3 | A black hole sits on the direct line |
| 7 The Long Drift | 5 | A long S-shaped fairway through nebula switchbacks |
| 8 Binary Star | 4 | Two planets; the cup opens away from you and the wind pushes into it |
| 9 Wormhole Home | 5 | Wind, a moon, a planet, a bumper and a black hole |

Course par is 31. The test bot, which plans with the exact physics, scores
18; par leaves a player without that foresight room to misread a curve.

## Rules and scoring

* A stroke ends when the ball has moved under 24 subpixels per step for ten
  steps, or after 1,200 steps.
* The ball sinks within 4 px of the cup centre if it is moving slowly enough
  (under 320 subpixels per step). Faster balls roll over the lip.
* Falling into a black hole wastes the stroke and returns the ball to where
  it was struck.
* After par + 3 strokes the ball is picked up and the hole scores par + 4.
* Points per hole: 100 × (par + 3 − strokes), plus 500 for a hole in one. Any
  scope charges left at the end are worth 100 each. You start with three
  charges and earn one (up to nine) for every hole under par.

## Physics

Positions are 1/256-pixel fixed point and velocities are 16-bit
subpixels per step. Two steps run per 30 Hz update. Each step:

1. Adds the current tile's precomputed force vector.
2. Applies drag: −(v/64 + 1) on open space, −(v/8 + 2) in nebula, clamped
   to ±1,024.
3. Moves along x, then along y. It probes 2 px ahead and bounces off rock and
   planets at ¾ speed, or off bumpers at 1¼ speed plus a kick, capped at 900.
4. Checks the cup, the black hole and the rest condition.

`tools/course.py` holds the course, drawn as 20 × 15 character grids. It
folds every planet (1/d), black hole and cup (1/d²) and wind tile into one
signed-byte force vector per tile. The cartridge's physics is therefore
integer addition and table lookups, with no division or square roots on the
device. The same file contains a line-for-line Python copy of the physics
step. It checks that each hole is completable, and the test bot uses it to
plan strokes.

## Art and sound

Asteroid walls use three rubble tiles picked by a position hash. Planets
reuse the shared 32 × 32 planet tiles; moons are 16 × 16. Nebula is a violet
dither and wind is dim amber chevrons. The ball leaves a three-sprite trail,
the cup has a waving pennant, and black holes spin.

Sound: a soft tick as power changes, a strike whose pitch rises with power,
a bright ping off bumpers, a low rumble into the void, and a chime on sinking.

## Budgets

| Resource | Use |
| --- | --- |
| ROM | 32 KiB cartridge, 23.4 KiB used; the course is 8.1 KiB of it |
| Work RAM | 130 bytes |
| Background tiles | 21 custom plus the shared planet |
| Sprites | ball 1, trail 3, guide or scope 16, pennant 1, black hole 1, celebration 4 |
| Frame time | The HUD redraws changed digits only; one physics step costs roughly 2,000 CPU cycles, so the scope traces five per update |

## Verification

The test bot plans each stroke with the Python physics, then aims with the
D-pad and putts. After every stroke the ball's resting position on the
cartridge must match the Python prediction exactly. The recorded run had
zero mismatches over the whole course. The test also rotates the aim with
the scope active for 120 frames and requires the full 60 updates.
