"""Website artwork for Dreambase Invaders.

If art/cover-illustrated-v1.png exists (promotional key art), the card,
detail and share images are cut from it. Otherwise a pixel poster is drawn at
256 x 171 from the cartridge's own sprites, marks and palette, scaled six
times with nearest-neighbour sampling, and used instead.

Run with .tools/venv/bin/python games/dreambase-invaders/tools/cover.py.
"""
from pathlib import Path
import math
import random
import sys

from PIL import Image

HERE = Path(__file__).resolve().parent
ART = HERE.parent / 'art'
sys.path.insert(0, str(HERE))
import assets  # noqa: E402
from art_data import MARK24, WORD14  # noqa: E402

W, H, SCALE = 256, 171, 6


def c8(c):
    return tuple(min(255, v * 8 + v // 4) for v in c)


class Poster:
    def __init__(self):
        self.img = Image.new('RGB', (W, H), c8(assets.INK))
        self.px = self.img.load()

    def dot(self, x, y, color):
        if 0 <= x < W and 0 <= y < H:
            self.px[int(x), int(y)] = color

    def grid(self, rows, x0, y0, pal, scale=1):
        for y, row in enumerate(rows):
            for x, v in enumerate(row):
                if v:
                    for a in range(scale):
                        for b in range(scale):
                            self.dot(x0 + x * scale + a, y0 + y * scale + b, c8(pal[v]))


def poster():
    P = Poster()
    rnd = random.Random(0xD12EA)
    # Sky: a faint green glow rising from the base.
    for y in range(H):
        for x in range(W):
            d = math.hypot((x - W / 2) / 1.6, y - 150)
            g = max(0.0, 1 - d / 110)
            P.dot(x, y, c8((0, int(1 + 7 * g * g), int(2 + 4 * g * g))))
    for _ in range(140):
        x, y = rnd.randrange(W), rnd.randrange(120)
        P.dot(x, y, c8(assets.STARS[rnd.choice([1, 2, 2, 3])]))
    # Wordmark and INVADERS, centred.
    mark = assets.parse(MARK24)
    word = assets.parse(WORD14)
    total = 24 + 6 + len(word[0])
    x0 = (W - total) // 2
    P.grid(mark, x0, 6, assets.GREEN_TEXT)
    P.grid(word, x0 + 30, 11, assets.TEXT)
    # INVADERS at three times the cartridge font, with the copper gradient.
    cols = [c[1] for c in assets.COMPANIES]
    word, pitch, sc = 'INVADERS', 25, 3
    left = (W - (len(word) * pitch - 4)) // 2
    face = set()
    for i, ch in enumerate(word):
        g = assets.glyph(ch)
        for y in range(7):
            for x in range(7):
                if g[y][x]:
                    for a in range(sc):
                        for b in range(sc):
                            face.add((left + i * pitch + x * sc + a, 34 + y * sc + b))
    for x, y in face:
        for d in (1, 2, 3):
            if (x + d // 2, y + d) not in face:
                P.dot(x + d // 2, y + d, c8((3, 4, 9)))
    for x, y in face:
        k = (y - 34) / 21 * 3 + 2.5
        a, b = cols[int(k) % 8], cols[(int(k) + 1) % 8]
        f = k - int(k)
        top = (x, y - 1) not in face
        col = (31, 31, 31) if top else tuple(round(a[j] + (b[j] - a[j]) * f) for j in range(3))
        P.dot(x, y, c8(col))
    # The partner invaders fan out around the Dreambase hero.
    spots = [(1, 34, 92), (3, 8, 112), (2, 196, 78), (4, 214, 106), (5, 52, 124),
             (6, 172, 118), (7, 226, 136), (0, 6, 138)]
    for form, x, y in spots:
        if form == 0:
            continue
        P.grid(assets.invader(form, 0), x, y - 8, assets.FORMS[form], 2)
    P.grid(assets.invader(0, 1), 104, 66, assets.FORMS[8], 3)
    # Data bits rising into its mouth.
    for i, (x, y) in enumerate([(116, 132), (132, 146), (98, 150), (150, 128)]):
        P.grid(assets.logo(i * 2 + 1 if i < 3 else 0), x, y, assets.LOGO_PALS[i * 2 + 1 if i < 3 else 0])
    # The lunar base on the horizon.
    surf = assets.surface_picture()
    for x in range(W):
        for y in range(8):
            v = surf[y][x % 160]
            P.dot(x, 158 + y, c8(assets.MOON[v]))
        for y in range(166, H):
            P.dot(x, y, c8((2, 3, 5)))
    dome = assets.dome_picture()
    P.grid(dome, (W - 48) // 2, 134, assets.BASE)
    cannon = assets.cannon_picture()
    for i, x in enumerate([60, 84, 156, 180]):
        P.grid(cannon, x, 142, assets.cannon_pal([0, 1, 3, 4][i]))
    for x in range(W):
        for y in range(160, H):
            P.dot(x, y, c8((2, 3, 5)) if y > 161 else c8(assets.MOON[1]))
    # Tagline.
    tag = 'EAT THE DATA'
    tx = (W - len(tag) * 8) // 2
    for i, ch in enumerate(tag):
        P.grid(assets.font_tile(ch), tx + i * 8, 163, assets.GREEN_TEXT)
    for x in range(tx - 40, tx - 8):
        P.dot(x, 166, c8(assets.DREAM_DARK))
        P.dot(W - x - 1, 166, c8(assets.DREAM_DARK))
    return P.img


def main():
    source = ART / 'cover-illustrated-v1.png'
    if source.exists():
        im = Image.open(source).convert('RGB')
    else:
        im = poster().resize((W * SCALE, H * SCALE), Image.NEAREST).crop((0, 0, 1536, 1024))
        im.save(ART / 'cover.png', optimize=True)
    for kind, width in (('card', 828), ('detail', 972), ('share', 1200)):
        out = ART / f'cover-{kind}-v1.{"jpg" if kind == "share" else "webp"}'
        resized = im.resize((width, round(im.height * width / im.width)),
                            Image.Resampling.NEAREST if not source.exists() else Image.Resampling.LANCZOS)
        if kind == 'share':
            resized.save(out, quality=88, optimize=True, progressive=True)
        else:
            resized.save(out, quality=88, method=6)
        print(out.name, resized.size, out.stat().st_size)


if __name__ == '__main__':
    main()
