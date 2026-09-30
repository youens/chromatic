# PRISM WELL artwork

`cover.jpg` is a pixel-art poster composed on September 29, 2026 by
[../../../tools/pixel_covers.py](../../../tools/pixel_covers.py). It is drawn at
256 × 171 pixels from the cartridge's own tiles, sprites and palette plus a few
simple shapes and the cartridge font, then scaled six times with
nearest-neighbour sampling and cropped to 1536 × 1024. Unlike the covers of the
first five anthology games, it is not a generated illustration. Run
`.tools/venv/bin/python tools/pixel_covers.py` from the repository root to
rebuild it.

`title.png`, `gameplay.png` and `results.png` are captures of the actual
compiled GBC cartridge, taken by `tools/test_collection.py`. Native pixel assets
are authored in `../../shared/worlds.py`.
