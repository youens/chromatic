"""Compose pixel-art covers for Stormkite, Comet Links and Prism Well.

Each cover is drawn at 256 x 171 virtual pixels from the cartridge's own
tiles, sprites and palette, then scaled six times with nearest-neighbour
sampling to 1536 x 1024. Unlike the earlier covers, these are not generated
illustrations: every pixel comes from the game's native art or from the simple
shapes below.
"""
from pathlib import Path
import random
import re
import sys

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'games/shared'))
import worlds  # noqa: E402

W, H, SCALE = 256, 171, 6
FONT = eval(re.search(r'FONT = (\{.*?\n\})', (ROOT / 'games/shared/assets.py').read_text(), re.S).group(1))


def palette(slug):
    source = (ROOT / 'games' / slug / 'src/main.c').read_text()
    body = re.search(r'game_palette\[32\] = \{(.*?)\};', source, re.S).group(1)
    colors = [tuple(int(v) * 255 // 31 for v in m) for m in re.findall(r'C\((\d+), (\d+), (\d+)\)', body)]
    assert len(colors) == 32, slug
    return [colors[i * 4:i * 4 + 4] for i in range(8)]


class Poster:
    def __init__(self, background):
        self.img = Image.new('RGB', (W, H), background)
        self.px = self.img.load()

    def dot(self, x, y, color):
        if 0 <= x < W and 0 <= y < H:
            self.px[int(x), int(y)] = color

    def pattern(self, rows, x, y, pal, scale=1, flip=False):
        """Draw a grid of colour indices; index 0 is transparent."""
        for ry, row in enumerate(rows):
            for rx, v in enumerate(row[::-1] if flip else row):
                if v:
                    for dy in range(scale):
                        for dx in range(scale):
                            self.dot(x + rx * scale + dx, y + ry * scale + dy, pal[v])

    def text(self, s, x, y, color, scale=1, shadow=None):
        if shadow:
            self.text(s, x + scale, y + scale, shadow, scale)
        for ch in s:
            rows = [[1 if c == '1' else 0 for c in r] for r in FONT.get(ch, ['00000'] * 7)]
            self.pattern(rows, x, y, [None, color], scale)
            x += 6 * scale

    def disc(self, cx, cy, r, color):
        for y in range(int(cy - r), int(cy + r) + 1):
            for x in range(int(cx - r), int(cx + r) + 1):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                    self.dot(x, y, color)

    def save(self, path):
        big = self.img.resize((W * SCALE, H * SCALE), Image.NEAREST).crop((0, 0, 1536, 1024))
        big.save(path, quality=92, optimize=True)
        print('wrote', path.relative_to(ROOT))


def decode(tile):
    rows = []
    for y in range(8):
        lo, hi = tile[y * 2], tile[y * 2 + 1]
        rows.append([((lo >> (7 - x)) & 1) | (((hi >> (7 - x)) & 1) << 1) for x in range(8)])
    return rows


def stars(poster, colors, count, seed, top=0, bottom=H):
    rng = random.Random(seed)
    for i in range(count):
        poster.dot(rng.randrange(W), rng.randrange(top, bottom), colors[i % len(colors)])


def stormkite():
    pal = palette('stormkite')
    night = pal[0][0]
    poster = Poster(night)
    # Sky fading from night into the dusk band, dithered like the cartridge.
    for y in range(0, 60):
        for x in range(W):
            t = y / 60
            if (x + y) % 2 == 0 and ((x * 7 + y * 3) % 11) / 11 < t * 0.7:
                poster.dot(x, y, pal[5][1])
    stars(poster, [pal[4][3], pal[4][2]], 70, 5, 0, 50)
    bg = [[0] * 16 for _ in range(192)]
    scene, colors, _ = worlds.stormkite_world(bg)
    top = H - 120
    for row in range(2, 17):
        for col in range(32):
            n = row * 32 + col
            rows = decode(bg[scene[n]])
            attr = colors[n]
            if attr & 0x20:
                rows = [r[::-1] for r in rows]
            if attr & 0x40:
                rows = rows[::-1]
            p = pal[attr & 7]
            for y in range(8):
                for x in range(8):
                    if rows[y][x] or attr & 7 != 5:
                        poster.dot(col * 8 + x, top + (row - 2) * 8 + y, p[rows[y][x]])
    boss = worlds.THUNDERHEAD
    poster.pattern(boss, 176, 18, pal[3], 3)
    for i, (x, y) in enumerate([(196, 90), (190, 100), (198, 110), (192, 122)]):
        poster.pattern(worlds.grid(worlds.STORMKITE_SPRITES[3]), x, y, pal[3], 2)
    for x, y in [(150, 70), (166, 104), (130, 92), (210, 132)]:
        poster.pattern(worlds.grid(worlds.STORMKITE_SPRITES[1]), x, y, pal[3], 2)
    poster.pattern(worlds.STORMKITE_HERO, 54, 72, pal[1], 3)
    for i in range(4):
        poster.pattern(worlds.grid(worlds.STORMKITE_SPRITES[5]), 44 - i * 11, 118 + (i % 2) * 5 - i * 2, pal[1], 2)
    for i in range(3):
        poster.pattern(worlds.grid(worlds.STORMKITE_SPRITES[0]), 108 + i * 14, 94 - i * 6, pal[1], 2)
    poster.pattern(worlds.grid(worlds.STORMKITE_SPRITES[4]), 92, 128, pal[6], 2)
    poster.text('STORMKITE', 12, 10, pal[1][3], 3, shadow=pal[1][1])
    poster.text('RIDE THE THUNDER', 14, 38, pal[5][3])
    return poster


def comet_links():
    pal = palette('comet-links')
    poster = Poster(pal[0][0])
    stars(poster, [pal[4][3], pal[4][2], pal[1][3]], 120, 3)
    # A large banded planet, shaded like the cartridge's planet tiles.
    cx, cy, r = 62, 118, 44
    for y in range(int(cy - r), int(cy + r) + 1):
        for x in range(int(cx - r), int(cx + r) + 1):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                light = x + y < cx + cy + 18
                band = ((x * 3 + y * 7) // 22) % 3
                poster.dot(x, y, pal[5][3] if light and band else pal[5][2] if light else pal[5][1])
    poster.disc(214, 42, 12, pal[5][2])
    poster.disc(218, 38, 8, pal[5][3])
    # A slingshot putt: scope dots ahead of the comet, its tail behind.
    def arc(t):
        return ((1 - t) ** 2 * 30 + 2 * t * (1 - t) * 150 + t * t * 206,
                (1 - t) ** 2 * 96 + 2 * t * (1 - t) * 8 + t * t * 118)
    for i in range(12):
        x, y = arc(0.6 + i * 0.4 / 12)
        poster.pattern(worlds.grid(worlds.COMET_SPRITES[4]), x - 4, y - 4, pal[2])
    for i in range(10):
        x, y = arc(0.36 + i * 0.022)
        poster.disc(x, y, 1 + i // 4, pal[1][2 if i < 6 else 3])
    hx, hy = arc(0.58)
    ex, ey = 206, 118
    cup = worlds.grid(worlds.COMET_ART['cup'])
    poster.pattern(cup, ex - 12, ey - 12, pal[2], 3)
    poster.pattern(worlds.grid(worlds.COMET_SPRITES[5]), ex - 2, ey - 40, pal[2], 3)
    poster.disc(hx, hy, 4, pal[1][3])
    for i in range(10):
        poster.pattern(worlds.grid(worlds.COMET_ART['rock'][i % 3]), i * 26 - 4, 150 + (i % 2) * 6, pal[6], 3)
    poster.text('COMET', 12, 10, pal[1][3], 3, shadow=pal[1][1])
    poster.text('LINKS', 12, 36, pal[2][3], 3, shadow=pal[2][1])
    poster.text('EVERY PLANET PULLS', 14, 62, pal[4][3])
    return poster


def prism_well():
    pal = palette('prism-well')
    poster = Poster(pal[0][0])
    wall = worlds.grid(worlds.PRISM_ART['wall'])
    for y in range(0, H, 24):
        for x in (118, 238):
            poster.pattern(wall, x, y, pal[4], 3, flip=x > 200)
    for y in range(0, H, 8):
        for x in range(0, 110, 8):
            if (x * 5 + y * 3) % 29 == 0:
                poster.pattern(worlds.grid(worlds.PRISM_ART['dust']), x, y, pal[4], 1)
    gems = worlds.PRISM_GEMS
    pal_of = [1, 2, 3, 5, 6]
    layout = [
        '..1..', '.32..', '4213.', '52145', '13524', 'SS.SS',
    ]
    for r, row in enumerate(layout):
        for c, ch in enumerate(row):
            x, y = 142 + c * 18, 62 + r * 18
            if ch == 'S':
                poster.pattern(worlds.grid(worlds.PRISM_ART['stone']), x, y, pal[7], 2)
            elif ch != '.':
                g = int(ch) - 1
                poster.pattern(worlds.grid(gems[g]), x, y, pal[pal_of[g]], 2)
    for i, g in enumerate([0, 3, 4]):
        poster.pattern(worlds.grid(gems[g]), 178, 4 + i * 18, pal[pal_of[g]], 2)
    for x, y in [(176, 80), (194, 98)]:
        poster.pattern(worlds.grid(worlds.PRISM_ART['flash']), x, y, pal[0], 2)
    poster.pattern(worlds.PRISM_HERO, 34, 92, pal[1], 4)
    poster.text('PRISM', 12, 10, pal[1][3], 3, shadow=pal[1][1])
    poster.text('WELL', 12, 36, pal[2][2], 3, shadow=pal[2][1])
    poster.text('LINE UP THE LIGHT', 14, 62, pal[4][3])
    return poster


if __name__ == '__main__':
    for slug, make in [('stormkite', stormkite), ('comet-links', comet_links), ('prism-well', prism_well)]:
        make().save(ROOT / 'games' / slug / 'art/cover.jpg')
