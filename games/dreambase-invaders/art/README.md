# Dreambase Invaders artwork

`cover.png` is a pixel poster drawn by [../tools/cover.py](../tools/cover.py)
at 256 × 171 pixels from the cartridge's own sprites, Dreambase mark,
wordmark and palette, then scaled six times with nearest-neighbour sampling
and cropped to 1536 × 1024. Its layout follows the Dreambase Invaders share
image: the wordmark and INVADERS above the green Dreambase invader, the
partner invaders around it, the glowing base, and the tagline EAT THE DATA.
`cover-card-v1.webp`, `cover-detail-v1.webp` and `cover-share-v1.jpg` are the
website's sizes of the same image.

To use illustrated key art instead, save it as `cover-illustrated-v1.png`
(1536 × 1024 or any 3:2 size) and run
`.tools/venv/bin/python games/dreambase-invaders/tools/cover.py` from the
repository root. The script cuts the three website sizes from the
illustration when that file exists. Then set `"cover": "illustrated"` for
this game in `tools/games.json`.

`splash.png`, `title.png`, `help.png`, `stack.png`, `intro.png`,
`gameplay.png`, `levelup.png`, `late-level.png`, `credits.png`, `results.png`
and `game-over.png` are captures of the compiled cartridge, taken by
[../tools/test.py](../tools/test.py).
