# Dotwing native art

Every ROM tile and sprite is drawn at native resolution by the Python in
`../tools/`. Run `make -C games/dotwing all` from the repository root (or
`python games/dotwing/tools/assets.py` with Pillow installed) to rebuild the
generated headers in `../src/` and these previews:

| File | Contents |
| --- | --- |
| `native-contact-sheet.png` | Plane, shots, bullets, pickups, explosions, the four rival fleets and the four bosses (3x) |
| `plane-native.png` | The player plane in its default livery |
| `grok-native.png`, `claude-native.png`, `gemini-native.png`, `muse-native.png` | Each lab's drone, gunner and heavy craft, two animation frames each |
| `boss-1-native.png` to `boss-4-native.png` | The 96x56 background-layer bosses as they appear on screen |
| `level-1.png` to `level-4.png` | Each sector's full 24x300-tile scroll, from the launch pad (bottom) to the boss arena (top) |
| `dot-builder-sheet.png` | Every dot shape, face and gear combination, and the title plane (3x) |
| `gameplay.png` | An actual emulator capture of sector 2 |

Sprites are 2bpp in 8x16 column-major layout: small craft 16x16, heavy
craft 32x16. Scenery is assembled from stamps whose tiles are deduplicated
with flips. The four lab motifs are playful interpretations, not exact
official logos.

`plane-reference-v1.png`, `plane-grid-reference-v1.png` and
`plane-blank-grid.png` are Image Generation references from the first
release; no generated reference was ever downsampled into a ROM sprite.
The illustrated covers are promotional artwork; see `promo-v2.md`.
