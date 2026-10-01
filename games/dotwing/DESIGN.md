# Dotwing design

The fast movement and readable attack lanes of 1942 and Sky Force inspire an
offline arcade flight game with an AI theme. A friendly customizable dot
anchors the game's identity. Colorful clouds, circuit architecture, livery
colors, bright pickups and large rival emblems make the small display readable.

The loop is build a dot, outfit a plane, launch, dodge and chain rival defeats,
collect boosts, defeat a sector boss, upgrade at a sky station, and challenge
the next lab. Collected tokens persist even after a failed run. Four sectors
complete a campaign; best score gives replay value.

| Sector | Rival craft | Attack identity | Environment |
| --- | --- | --- | --- |
| 1 | Grok orbital slash | Zigzag drones; three-lane boss fan | Azure clouds |
| 2 | Claude sunburst | Paired drone shots; four-way boss flare | Coral coast |
| 3 | Gemini star | Weaving craft; crossed boss streams | Circuit city |
| 4 | Muse M | Swooping craft; aimed boss volleys | Violet nexus |

Native hardware supplies 160x144 pixels, eight background palettes, eight
sprite palettes, and forty sprites. A small collision core and hold-A focus
movement make busy attacks fair. The B burst gives a timed escape, while
token boosts create short windows of rapid fire and aggressive scoring.

Bounded pools contain six drones, eight player shots, eight hostile shots,
four pickups, and four effects. Bosses replace drones in the OAM budget.
Backgrounds scroll through a 256-pixel seamless tile map; the window anchors
the HUD. Background tiles use signed addressing at 0x9000, leaving sprite
graphics at 0x8000 intact. Artwork is authored directly at native resolution.

Each new module fits a 16 KiB ROM bank. The anthology assigns banks26–30 to
Dotwing and bank31 to launcher save handling; all pre-existing games remain.
Art scratch buffers overlay the inactive shared screen maps at D000, while
a fixed text bridge copies strings into DF00 before changing ROM bank.

Persistent customization and progression use a versioned, bounds-checked
two-slot save with a rolling generation and checksum. Tokens are credited
once per run checkpoint; leaving a shop cannot repeat its reward. Cartridge
save regions do not overlap. Browser SRAM is namespaced by game slug and
restored before boot.
