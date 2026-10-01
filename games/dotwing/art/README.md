# Dotwing native art

`../tools/assets.py` is the editable source of every ROM tile and promotional
pixel. Run `python3 games/dotwing/tools/assets.py` from the repository root to
rebuild `src/art.c`, native RGBA sources, the contact sheet and optimized covers.
Sprites are 2bpp, in 8x16 column-major layout; small actors16x16, bosses32x32.
The four lab motifs are playful interpretations, not exact official logos.

`plane-reference-v1.png` is an Image Generation silhouette reference. The
second `plane-grid-reference-v1.png` attempted an exact16x16 chart but actually
has18x18 cells, so recovery was rejected. Native cells were deliberately authored
in the generator, retaining the broad swept-wing silhouette and cockpit.
No generated reference was downsampled into a ROM sprite. The contact sheet is
a source-art preview; gameplay images are actual emulator captures when present.
