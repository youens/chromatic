"""Build every tile, map and palette for Dreambase Invaders.

Usage: assets.py OUTPUT_HEADER [PREVIEW_PNG]

The header is included by src/art.c, which owns the cartridge bank that holds
the graphics. Grids live in art_data.py; logos, the lunar base and the title
lettering are drawn here procedurally so their geometry stays editable.

VRAM layout (BG tiles use the 0x8800 addressing mode, GBDK's default):
  OBJ bank 0   0-63 invader forms, 64-95 partner logos, 96-111 sparks
  OBJ bank 1   0-29 popup glyphs, 32-95 title formation (recoloured at load)
  BG  bank 0   0 blank, 1-59 font, 60-95 HUD and stars
  BG  bank 1   0-79 tall font (doubled from the font at load)
  shared 128+  per-screen art: lunar base, title lettering, splash
"""
from pathlib import Path
import math
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from art_data import FONT, MARK48, MARK24, MARK16, MARK8, WORD14, INVADERS, POP  # noqa: E402


# ------------------------------------------------------------------ basics
def canvas(w, h, c=0):
    return [[c] * w for _ in range(h)]


def parse(rows, width=None):
    width = width or len(rows[0])
    out = []
    for r in rows:
        assert len(r) == width, (len(r), width, r)
        out.append([0 if ch in '. ' else int(ch) for ch in r])
    return out


def mirror(rows):
    return [r[::-1] for r in rows]


def put(p, x, y, c):
    if 0 <= y < len(p) and 0 <= x < len(p[0]):
        p[y][x] = c


def blit(dst, src, x0, y0, transparent=True):
    for y, row in enumerate(src):
        for x, v in enumerate(row):
            if v or not transparent:
                put(dst, x0 + x, y0 + y, v)


def encode(tile):
    out = []
    for row in tile:
        lo = hi = 0
        for v in row:
            lo = (lo << 1) | (v & 1)
            hi = (hi << 1) | ((v >> 1) & 1)
        out += [lo, hi]
    return out


def cut(p, x, y):
    return [r[x:x + 8] for r in p[y:y + 8]]


def sprite16(p):
    """8 x 16 sprite order: top-left, bottom-left, top-right, bottom-right."""
    return [encode(cut(p, 0, 0)), encode(cut(p, 0, 8)), encode(cut(p, 8, 0)), encode(cut(p, 8, 8))]


def sprite8(p8):
    """An 8 x 8 picture for an 8 x 16 sprite; art.c adds the blank half."""
    return [encode(p8)]


def rgb(r, g, b):
    return r | (g << 5) | (b << 10)


def hexc(h):
    h = h.lstrip('#')
    return tuple(int(h[i:i + 2], 16) >> 3 for i in (0, 2, 4))


def outline(fill, colour, diagonal=False):
    """Add a one-pixel border of `colour` around non-zero pixels."""
    h, w = len(fill), len(fill[0])
    out = [r[:] for r in fill]
    steps = [(1, 0), (-1, 0), (0, 1), (0, -1)] + ([(1, 1), (-1, -1), (1, -1), (-1, 1)] if diagonal else [])
    for y in range(h):
        for x in range(w):
            if fill[y][x]:
                continue
            if any(0 <= x + dx < w and 0 <= y + dy < h and fill[y + dy][x + dx] for dx, dy in steps):
                out[y][x] = colour
    return out


# -------------------------------------------------------------------- font
def glyph(ch):
    return [[1 if c == '#' else 0 for c in row] for row in FONT[ch]]


def font_tile(ch):
    """7 x 7 glyph, upper rows bright (3), lower rows softer (2), shadow 1."""
    g = glyph(ch)
    t = canvas(8, 8)
    for y in range(7):
        for x in range(7):
            if g[y][x]:
                t[y][x] = 3 if y < 4 else 2
    for y in range(7):
        for x in range(7):
            if g[y][x] and not t[y + 1][x + 1]:
                t[y + 1][x + 1] = 1
    return t


FONT_CHARS = [chr(c) for c in range(32, 91)]
for ch in FONT_CHARS:
    assert ch in FONT, ch
TALL_CHARS = ' ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!.:+'


# ---------------------------------------------------------------- palettes
INK = hexc('#04060c')
WHITE = (31, 31, 31)
DREAM = hexc('#14EC77')
DREAM_DARK = hexc('#00623A')
DREAM_MID = hexc('#3CCE8E')
GOLD = hexc('#ffd24a')
DANGER = hexc('#ff3355')
MOON_LIGHT, MOON_MID, MOON_DARK = hexc('#8b93a7'), hexc('#5c6373'), hexc('#343a47')
DEEP = (2, 3, 5)
METAL, EDGE = hexc('#1a2130'), hexc('#2e3a52')

# Level companies in play order, with their brand colours.
COMPANIES = [
    # name, colour, dark, light
    ('SUPABASE', hexc('#3ECF8E'), hexc('#1c6b49'), (20, 31, 25)),
    ('STRIPE', hexc('#635BFF'), (6, 5, 18), (22, 21, 31)),
    ('POSTHOG', hexc('#F54E00'), (12, 3, 0), (31, 23, 5)),
    ('VERCEL', WHITE, (8, 9, 14), (17, 20, 31)),
    ('GITHUB', hexc('#8957E5'), (8, 5, 15), (27, 22, 31)),
    ('CLICKHOUSE', hexc('#FAFF69'), (14, 14, 4), (31, 31, 26)),
    ('LINEAR', hexc('#5E6AD2'), (5, 6, 14), (22, 24, 31)),
    ('DREAMBASE', DREAM, DREAM_DARK, (20, 31, 25)),
]

# Player form palettes [transparent, shade, body, highlight]. Form 8 is the
# final Dreambase form, which reuses form 0's tiles.
FORMS = [
    [INK, (3, 4, 9), (24, 27, 31), WHITE],         # Data Analyst
    [INK, (1, 8, 6), (7, 26, 18), WHITE],          # Supabase
    [INK, (3, 3, 12), (13, 12, 31), WHITE],        # Stripe
    [INK, (8, 2, 0), (31, 10, 1), (31, 29, 22)],   # PostHog
    [INK, (3, 3, 7), (29, 30, 31), (17, 20, 31)],  # Vercel
    [INK, (4, 4, 6), (21, 21, 23), WHITE],         # GitHub
    [INK, (8, 7, 0), (31, 30, 8), WHITE],          # ClickHouse
    [INK, (3, 4, 12), (12, 14, 28), WHITE],        # Linear
    [INK, (0, 10, 5), DREAM, (24, 31, 27)],        # Dreambase final form
]
FORM_TILES = [0, 1, 2, 3, 4, 5, 6, 7, 0]

LOGO_PALS = [
    [INK, hexc('#1c6b49'), hexc('#3ECF8E'), (20, 31, 25)],
    [INK, (6, 5, 18), hexc('#635BFF'), WHITE],
    [INK, hexc('#1D4AFF'), hexc('#F54E00'), hexc('#F9BD2B')],
    [INK, (8, 10, 22), (17, 20, 31), WHITE],
    [INK, (10, 6, 18), hexc('#8957E5'), WHITE],
    [INK, (31, 2, 2), (22, 22, 6), hexc('#FAFF69')],
    [INK, (5, 6, 14), hexc('#5E6AD2'), WHITE],
    [INK, DREAM_DARK, DREAM, (20, 31, 25)],
]

STARS = [INK, (8, 9, 14), (19, 20, 24), (27, 28, 31)]
TEXT = [INK, (4, 5, 9), (20, 23, 29), WHITE]
GREEN_TEXT = [INK, DREAM_DARK, DREAM_MID, DREAM]
GOLD_TEXT = [INK, (9, 6, 1), (26, 19, 4), GOLD]
MOON = [INK, MOON_DARK, MOON_MID, MOON_LIGHT]
BASE = [INK, METAL, EDGE, DREAM]
CANNON_FLASH = [INK, METAL, EDGE, WHITE]


def cannon_pal(i):
    return [INK, METAL, EDGE, COMPANIES[i][1]]


# --------------------------------------------------------------- sprites
def logo(i):
    p = canvas(16, 16)
    if i == 0:  # Supabase: two offset halves of a lightning bolt
        for y in range(1, 10):
            for x in range(9 - y, 9):
                p[y][x] = 2
        for y in range(6, 15):
            for x in range(7, 16 - (y - 6) - 1 + 1):
                if x <= 15 - (y - 6) and not p[y][x]:
                    p[y][x] = 1
        for y in range(1, 10):
            put(p, 9 - y, y, 3)
    elif i == 1:  # Stripe: a bold dollar sign with a white rim
        fill = parse([
            '................',
            '.......22.......',
            '....22222222....',
            '...2222222222...',
            '...222.22..22...',
            '...222.22.......',
            '...222222222....',
            '....222222222...',
            '.......22.2222..',
            '...22..22..222..',
            '...222.22..222..',
            '...2222222222...',
            '....22222222....',
            '.......22.......',
            '................',
            '................',
        ])
        for y in range(16):
            for x in range(16):
                if fill[y][x] and (y == 15 or not fill[y + 1][x]):
                    fill[y][x] = 1
        p = outline(fill, 3)
    elif i == 2:  # PostHog: three slanted bars, blue, orange, yellow
        for b in range(3):
            x0 = 1 + b * 5
            for x in range(x0, x0 + 4):
                for y in range(2 + (x - x0), 14):
                    p[y][x] = b + 1
    elif i == 3:  # Vercel: a white triangle with a blue glow
        fill = canvas(16, 16)
        for y in range(2, 14):
            half = (y - 1) * 0.55
            for x in range(16):
                if abs(x - 7.5) <= half:
                    fill[y][x] = 3
        p = outline(fill, 2)
        for x in range(16):
            if p[13][x] == 3:
                p[13][x] = 2
    elif i == 4:  # GitHub: the octocat inside its circle
        rows = [
            '................',
            '.....######.....',
            '...+########+...',
            '..+##########+..',
            '..###+####+###..',
            '.###+......####.',
            '.###........###.',
            '.###........###.',
            '.###........###.',
            '.###+......+###.',
            '.####+....+####.',
            '.+#+###..#####+.',
            '..##.....+####..',
            '...###....###...',
            '....+#+...##....',
            '................',
        ]
        outside = set()
        stack = [(0, 0)]
        while stack:
            x, y = stack.pop()
            if (x, y) in outside or not (0 <= x < 16 and 0 <= y < 16) or rows[y][x] != '.':
                continue
            outside.add((x, y))
            stack += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
        for y in range(16):
            for x in range(16):
                ch = rows[y][x]
                p[y][x] = 3 if ch == '#' else 2 if ch == '+' else 0 if (x, y) in outside else 1
    elif i == 5:  # ClickHouse: four bars, a short bar and a red block
        for b in range(4):
            x0 = 1 + b * 3
            for y in range(2, 14):
                p[y][x0] = 3
                p[y][x0 + 1] = 2
        for y in range(6, 10):
            p[y][13] = 3
            p[y][14] = 2
        for y in range(12, 14):
            p[y][1] = p[y][2] = 1
    elif i == 6:  # Linear: a disc whose lower-left half is striped
        fill = canvas(16, 16)
        for y in range(16):
            for x in range(16):
                if (x - 7.5) ** 2 + (y - 7.5) ** 2 <= 6.6 ** 2:
                    if x - y >= -1 or (y - x) % 3 == 1:
                        fill[y][x] = 3
                    else:
                        fill[y][x] = 1
        p = outline(fill, 2)
    elif i == 7:  # Dreambase: the mark
        p = parse(MARK16)
        for y in range(16):
            for x in range(16):
                if p[y][x] == 2 and y < 7 and x < 7 and (y == 0 or p[y - 1][x] in (0, 1)):
                    p[y][x] = 3
    return p


def invader(form, frame):
    rows = INVADERS[form][frame]
    p = parse(rows, 16)
    assert len(p) == 16, (form, frame)
    return p


def spark(kind):
    p = canvas(8, 8)
    if kind == 0:
        p[3][3] = 3
    elif kind == 1:
        for y, x in [(3, 3), (3, 4), (4, 3), (4, 4)]:
            p[y][x] = 3 if (y, x) == (3, 3) else 2
    elif kind == 2:
        for y, x in [(2, 3), (4, 3), (3, 2), (3, 4)]:
            p[y][x] = 2
        p[3][3] = 3
    elif kind == 3:
        for d in range(1, 3):
            for y, x in [(3 - d, 3), (3 + d, 3), (3, 3 - d), (3, 3 + d)]:
                p[y][x] = 2 if d == 2 else 3
        p[3][3] = 3
    elif kind == 4:  # dash streak
        for x in range(1, 7):
            p[3][x] = 2 if x < 4 else 3
    elif kind == 5:  # ring dot
        for y, x in [(2, 3), (2, 4), (5, 3), (5, 4), (3, 2), (4, 2), (3, 5), (4, 5)]:
            p[y][x] = 2
    elif kind == 6:  # muzzle puff
        for y in range(2, 6):
            for x in range(2, 6):
                if (x - 3.5) ** 2 + (y - 3.5) ** 2 < 4:
                    p[y][x] = 1 if y > 3 else 2
    elif kind == 7:  # pixel square chunk (data bit)
        for y in range(2, 5):
            for x in range(2, 5):
                p[y][x] = 3 if (x, y) == (2, 2) else 2
    return p


def pop_tile(ch):
    t = canvas(8, 8)
    for y, row in enumerate(POP[ch]):
        for x, c in enumerate(row):
            if c == '3':
                t[y][x] = 3
    for y in range(5):
        for x in range(3):
            if t[y][x] == 3 and t[y + 1][x + 1] == 0:
                t[y + 1][x + 1] = 1
    return t


POP_CHARS = '0123456789X!UP+'


# ---------------------------------------------------------- BG tile sets
class TileSet:
    """Deduplicating tile allocator (flips included) for one VRAM range."""

    def __init__(self, slots):
        self.slots = list(slots)  # (bank, index)
        self.tiles = []           # (bank, index, data)
        self.known = {}

    def add(self, tile, allow_flip=True):
        key = tuple(encode(tile))
        if key in self.known:
            return self.known[key]
        assert self.slots, 'out of tile slots'
        bank, index = self.slots.pop(0)
        self.tiles.append((bank, index, list(key)))
        attr = 0x08 if bank else 0
        self.known[key] = (index, attr)
        if allow_flip:
            for flip, variant in ((0x20, [r[::-1] for r in tile]), (0x40, tile[::-1]), (0x60, [r[::-1] for r in tile[::-1]])):
                self.known.setdefault(tuple(encode(variant)), (index, attr | flip))
        return self.known[key]

    def banks(self):
        out = {0: [], 1: []}
        for bank, index, data in self.tiles:
            out[bank].append((index, data))
        return out


def star_tiles():
    tiles = []
    t = canvas(8, 8); t[3][4] = 2; tiles.append(t)
    t = canvas(8, 8); t[4][3] = 3; t[3][3] = t[5][3] = t[4][2] = t[4][4] = 1; tiles.append(t)
    t = canvas(8, 8)
    for d in (1, 2):
        for y, x in [(3 - d, 3), (3 + d, 3), (3, 3 - d), (3, 3 + d)]:
            t[y][x] = 2 if d == 1 else 1
    t[3][3] = 3; tiles.append(t)
    t = canvas(8, 8); t[1][6] = 1; t[6][1] = 2; tiles.append(t)
    t = canvas(8, 8); t[5][5] = 3; tiles.append(t)
    return tiles


def bar_tile(fill):
    t = canvas(8, 8)
    for x in range(8):
        t[1][x] = t[6][x] = 1
        for y in range(2, 6):
            t[y][x] = (3 if y < 4 else 2) if x < fill else 0
    return t


def icon_heart():
    return parse(['.22.22..', '2332222.', '2322222.', '2222222.', '.22222..', '..222...', '...2....', '........'])


def icon_life():
    return parse(['.3....3.', '.333333.', '33.33.33', '33333333', '3.3..3.3', '3.3333.3', '..3..3..', '........'])


def mini_logo(i):
    shapes = [
        ['....3...', '...33...', '..333...', '.3333333', '3333333.', '...333..', '...33...', '...3....'],
        ['...3....', '.33333..', '33.3....', '.3333...', '...3.33.', '.33333..', '...3....', '........'],
        ['3...3...', '33..33..', '333.333.', '33333333', '33333333', '33333333', '........', '........'],
        ['...33...', '...33...', '..3333..', '..3333..', '.333333.', '.333333.', '33333333', '........'],
        ['.333333.', '33.33.33', '3......3', '3......3', '33....33', '.3333.3.', '..33.3..', '........'],
        ['3.3.3.3.', '3.3.3.3.', '3.3.3.33', '3.3.3.33', '3.3.3.3.', '3.3.3.3.', '........', '........'],
        ['..3333..', '.333333.', '3.333333', '.3.33333', '3.3.3333', '.3.3.33.', '..3.3...', '........'],
        ['..3333..', '.3...33.', '3.33..33', '3.333.33', '3.33..33', '.3...33.', '..3333..', '........'],
    ]
    return parse(shapes[i])


# ------------------------------------------------------------ lunar base
WIN_W, WIN_H = 20, 6
SLOTS = [24, 48, 80, 112, 136]   # cannon centre x
SIDE = [0, 1, 3, 4]              # slots with a barrel; slot 2 fires from the dome


def dome_picture():
    """48 x 24 picture of the dome, cols 7-12 and window rows 0-2."""
    p = canvas(48, 24)
    cx, base = 23.5, 24
    for y in range(24):
        for x in range(48):
            dx = (x - cx) / 23.5
            dy = (base - y) / 20.0
            if dx * dx + dy * dy <= 1.0:
                p[y][x] = 1
    for y in range(24):
        for x in range(48):
            if p[y][x] and any(not (0 <= x + a < 48 and 0 <= y + b < 24) or not p[y + b][x + a]
                               for a, b in ((1, 0), (-1, 0), (0, -1))):
                p[y][x] = 2
    for y in (10, 15, 20):
        for x in range(48):
            if p[y][x] == 1 and x % 2 == 0:
                p[y][x] = 2
    # window lights along the base ring
    for x in range(6, 42, 4):
        if p[22][x]:
            p[22][x] = 3
    # the Dreambase mark, 16 x 16, glowing on the front of the dome
    m = parse(MARK16)
    for y in range(16):
        for x in range(16):
            if m[y][x] == 2:
                p[y + 6][x + 16] = 3
            elif m[y][x] == 1:
                p[y + 6][x + 16] = 2
    # antenna mast and light, left of the hatch
    for y in range(2, 9):
        if not p[y][11]:
            p[y][11] = 2
    p[0][11] = p[1][11] = 3
    p[1][10] = p[1][12] = 2
    return p


def cannon_picture():
    """16 x 16 turret: barrel above, housing below. Colour 3 is the brand
    colour; a telegraphed shot swaps the palette so it flashes white."""
    return parse([
        '.....222222.....',
        '....23333332....',
        '.....231132.....',
        '.....233332.....',
        '.....231132.....',
        '.....233332.....',
        '....22222222....',
        '....21111112....',
        '...2222222222...',
        '..211111111112..',
        '..213333333312..',
        '.21111111111112.',
        '.21121111112112.',
        '2111111111111112',
        '2111111111111112',
        '2222222222222222',
    ])


def hatch_picture():
    p = canvas(16, 16)
    rows = parse([
        '....22222222....',
        '...2111111112...',
        '..211211112112..',
        '.21111111111112.',
        '2111111111111112',
        '2222222222222222',
    ])
    blit(p, rows, 0, 10)
    return p


def surface_picture():
    """160 x 8 lunar horizon strip for window row 3."""
    p = canvas(160, 8)
    import random
    rnd = random.Random(0xD12EA)
    for x in range(160):
        for y in range(8):
            p[y][x] = 3 if y < 2 else 2 if y < 6 else 1
        if rnd.random() < 0.18:
            p[rnd.randint(2, 6)][x] = 3 if rnd.random() < 0.5 else 1
    for cx, cy, rx, ry in [(10, 4, 5, 2), (34, 5, 4, 1.6), (95, 4, 6, 2), (126, 5, 4, 1.5), (150, 4, 5, 2)]:
        for y in range(8):
            for x in range(160):
                d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
                if d <= 1:
                    p[y][x] = 1
                elif d <= 1.6 and y < cy and x < cx + 1:
                    p[y][x] = 3
    for x in range(0, 160, 7):
        p[0][x] = 2
    return p


def build_base(ts, title=False):
    """Returns 20 x 6 tile and attribute maps for the lunar base.

    Palettes: 3 cannon flash, 4 stars, 5 moon, 6 base, 7 cannons. Rows 4-5
    are left blank for the HUD (gameplay) or the footer (title).
    """
    tiles = [[0] * WIN_W for _ in range(WIN_H)]
    attrs = [[4 if y < 3 else 0] * WIN_W for y in range(WIN_H)]
    # sparse static stars in the sky rows of the base
    star = star_tiles()
    for y in range(3):
        for x in range(WIN_W):
            if (x * 7 + y * 13) % 11 == 0:
                t, a = ts.add(star[(x + y) % 4])
                tiles[y][x], attrs[y][x] = t, a | 4
    dome = dome_picture()
    for ty in range(3):
        for tx in range(6):
            t, a = ts.add(cut(dome, tx * 8, ty * 8))
            # the dome hides a logo as it rises out of the hatch
            tiles[ty][7 + tx], attrs[ty][7 + tx] = t, a | 6 | 0x80
    surf = surface_picture()
    for tx in range(WIN_W):
        t, a = ts.add(cut(surf, tx * 8, 0))
        tiles[3][tx], attrs[3][tx] = t, a | 5
    return tiles, attrs


def cannon_tiles(ts):
    """Tile/attr quads (TL, TR, BL, BR) for cannon, lit cannon and hatch."""
    out = {}
    for name, pic in (('cannon', cannon_picture()), ('hatch', hatch_picture())):
        quad = []
        for ty in range(2):
            for tx in range(2):
                quad.append(ts.add(cut(pic, tx * 8, ty * 8)))
        out[name] = quad
    return out


# ---------------------------------------------------------- title art
def invaders_word():
    """INVADERS in doubled bold lettering with a dark extrusion: 160 x 24."""
    p = canvas(160, 24)
    word = 'INVADERS'
    pitch = 18
    x0 = (160 - (len(word) * pitch - 4)) // 2
    face = canvas(160, 24)
    for i, ch in enumerate(word):
        g = glyph(ch)
        for y in range(7):
            for x in range(7):
                if g[y][x]:
                    for a in range(2):
                        for b in range(2):
                            face[3 + y * 2 + a][x0 + i * pitch + x * 2 + b] = 1
    for y in range(24):
        for x in range(160):
            if face[y][x]:
                for d in (1, 2):
                    if y + d < 24 and x + d // 2 < 160 and not face[y + d][x + d // 2]:
                        p[y + d][x + d // 2] = 2
    for y in range(24):
        for x in range(160):
            if face[y][x]:
                p[y][x] = 3 if y == 0 or not face[y - 1][x] else 1
    return p


def wordmark_row():
    """Title rows 1-3: the mark (cols 1-3) and the wordmark (cols 4-19)."""
    mark = parse(MARK24)
    word = parse(WORD14)
    pm = canvas(24, 24)
    blit(pm, mark, 0, 0)
    pw = canvas(128, 24)
    blit(pw, word, (128 - len(word[0])) // 2 + 1, 5)
    return pm, pw


def splash_art():
    mark = parse(MARK48)
    word = parse(WORD14)
    return mark, word


# ------------------------------------------------------------ C output
def c_bytes(name, values, kind='uint8_t', storage='static '):
    body = ''.join('  ' + ','.join(str(v) for v in values[i:i + 24]) + ',\n' for i in range(0, len(values), 24))
    return f'{storage}const {kind} {name}[] = {{\n{body}}};\n'


def pal_words(pals):
    return [rgb(*c) for pal in pals for c in pal]


def main(out, preview=None):
    h = ['/* Generated by tools/assets.py. Do not edit. */', '#include <stdint.h>']
    defs = {}

    # Sprites, OBJ bank 0.
    obj0 = []
    for form in range(8):
        for frame in range(2):
            obj0 += sprite16(invader(form, frame))
    defs['SPR_LOGO'] = len(obj0)
    for i in range(8):
        obj0 += sprite16(logo(i))
    defs['SPR_SPARK'] = len(obj0)
    h.append(c_bytes('spr_tiles', sum(obj0, [])))
    defs['SPR_COUNT'] = len(obj0)
    sparks = []
    for k in range(8):
        sparks += sprite8(spark(k))
    h.append(c_bytes('spark_tiles', sum(sparks, [])))
    defs['SPARK_COUNT'] = len(sparks)

    # Popup glyphs, OBJ bank 1.
    obj1 = []
    for ch in POP_CHARS:
        obj1 += sprite8(pop_tile(ch))
    h.append(c_bytes('pop_tiles', sum(obj1, [])))
    defs['POP_COUNT'] = len(obj1)
    # Stars for the 32 x 32 playfield: (tile << 10) | (y << 5) | x.
    import random
    rnd = random.Random(0x5EED)
    stars = []
    for y in range(32):
        for x in range(32):
            if rnd.random() < 0.075:
                stars.append((y, x, rnd.choice([0, 0, 1, 1, 2, 3, 4])))
    colors_stars = [(t << 10) | (y << 5) | x for y, x, t in stars]
    defs['STAR_COUNT'] = len(stars)
    defs['SPR_TITLE'] = 32

    # Font and HUD tiles, BG bank 0 from tile 1.
    font = [encode(font_tile(ch)) for ch in FONT_CHARS[1:]]
    h.append(c_bytes('font_tiles', sum(font, [])))
    defs['FONT_COUNT'] = len(font)
    hud = []
    names = {}

    def hud_tile(name, t):
        names[name] = 60 + len(hud)
        hud.append(encode(t))
    for i, t in enumerate(star_tiles()):
        hud_tile(f'STAR{i}', t)
    for f in range(9):
        hud_tile(f'BAR{f}', bar_tile(f))
    hud_tile('HEART', icon_heart())
    hud_tile('LIFE', icon_life())
    for i in range(8):
        hud_tile(f'MINI{i}', mini_logo(i))
    h.append(c_bytes('hud_tiles', sum(hud, [])))
    defs['HUD_COUNT'] = len(hud)
    for k, v in names.items():
        defs['T_' + k] = v

    # Lunar base and cannons (shared by title and play), shared region bank 1.
    base_ts = TileSet([(1, i) for i in range(128, 256)])
    btiles, battrs = build_base(base_ts)
    quads = cannon_tiles(base_ts)
    base_tiles = base_ts.banks()[1]
    first = base_tiles[0][0]
    assert [i for i, _ in base_tiles] == list(range(first, first + len(base_tiles)))
    defs['BASE_FIRST'] = first
    defs['BASE_COUNT'] = len(base_tiles)
    h.append(c_bytes('base_tiles', sum((d for _, d in base_tiles), [])))
    h.append(c_bytes('base_map', sum(btiles, [])))
    h.append(c_bytes('base_attr', sum(battrs, [])))
    for name, quad in quads.items():
        h.append(c_bytes(f'cannon_{name}', [v for t, a in quad for v in (t, a | 7)]))

    # Title lettering, shared region bank 0.
    title_ts = TileSet([(0, i) for i in range(128, 256)])
    tmap = [[0] * 20 for _ in range(6)]
    tattr = [[0] * 20 for _ in range(6)]
    pm, pw = wordmark_row()
    for ty in range(3):
        for tx in range(3):
            t, a = title_ts.add(cut(pm, tx * 8, ty * 8))
            tmap[ty][1 + tx], tattr[ty][1 + tx] = t, a | 1
        for tx in range(16):
            t, a = title_ts.add(cut(pw, tx * 8, ty * 8))
            tmap[ty][4 + tx], tattr[ty][4 + tx] = t, a | 0
    inv = invaders_word()
    for ty in range(3):
        for tx in range(20):
            t, a = title_ts.add(cut(inv, tx * 8, ty * 8))
            tmap[3 + ty][tx], tattr[3 + ty][tx] = t, a | 2
    title_tiles = title_ts.banks()[0]
    defs['TITLE_FIRST'] = title_tiles[0][0]
    defs['TITLE_COUNT'] = len(title_tiles)
    h.append(c_bytes('title_tiles', sum((d for _, d in title_tiles), [])))
    h.append(c_bytes('title_map', sum(tmap, [])))
    h.append(c_bytes('title_attr', sum(tattr, [])))

    # Splash: the big mark and the wordmark, shared region bank 0.
    splash_ts = TileSet([(0, i) for i in range(128, 256)])
    mark, word = splash_art()
    smark = canvas(48, 48)
    blit(smark, mark, 0, 0)
    smap = []
    for ty in range(6):
        for tx in range(6):
            t, a = splash_ts.add(cut(smark, tx * 8, ty * 8))
            smap += [t, a]
    sword = canvas(128, 16)
    blit(sword, word, (128 - len(word[0])) // 2 + 1, 1)
    for ty in range(2):
        for tx in range(16):
            t, a = splash_ts.add(cut(sword, tx * 8, ty * 8))
            smap += [t, a]
    splash_tiles = splash_ts.banks()[0]
    defs['SPLASH_FIRST'] = splash_tiles[0][0]
    defs['SPLASH_COUNT'] = len(splash_tiles)
    h.append(c_bytes('splash_tiles', sum((d for _, d in splash_tiles), [])))
    h.append(c_bytes('splash_map', smap))

    # Shared colour tables live in the fixed bank (colors.h).
    colors = ['/* Generated by tools/assets.py. Do not edit. */', '#include <stdint.h>']
    colors.append(c_bytes('form_pals', pal_words(FORMS), 'uint16_t', storage=''))
    colors.append(c_bytes('logo_pals', pal_words(LOGO_PALS), 'uint16_t', storage=''))
    comp = []
    for name, col, dark, light in COMPANIES:
        comp += [col, dark, light]
    colors.append(c_bytes('company_cols', [rgb(*c) for c in comp], 'uint16_t', storage=''))
    fixed = {
        'pal_stars': STARS, 'pal_text': TEXT, 'pal_green': GREEN_TEXT, 'pal_gold': GOLD_TEXT,
        'pal_moon': MOON, 'pal_base': BASE, 'pal_flash': CANNON_FLASH,
    }
    for name, pal in fixed.items():
        colors.append(c_bytes(name, pal_words([pal]), 'uint16_t', storage=''))
    # The title's copper gradient: every partner colour in level order.
    copper = []
    cols = [c[1] for c in COMPANIES]
    for i in range(8):
        a0, b0 = cols[i], cols[(i + 1) % 8]
        for k in range(4):
            copper.append(rgb(*[round(a0[j] + (b0[j] - a0[j]) * k / 4) for j in range(3)]))
    colors.append(c_bytes('copper_cols', copper, 'uint16_t', storage=''))
    # Splash reveal order: mark tiles swept by angle from the ring's opening.
    order = []
    for ty in range(6):
        for tx in range(6):
            if any(v for r in cut(smark, tx * 8, ty * 8) for v in r):
                ang = math.atan2((tx - 2.5), -(ty - 2.5))
                order.append(((-ang) % (2 * math.pi), math.hypot(tx - 2.5, ty - 2.5), ty * 8 + tx))
    order.sort()
    init = {'SPLASH_ORDER': [o[2] for o in order]}
    defs['SPLASH_TILES'] = len(order)
    sine = [round(64 * math.sin(2 * math.pi * i / 32)) for i in range(32)]
    init['SINE32'] = [v & 255 for v in sine]
    init['STAR_LIST'] = colors_stars
    defs['TALL_COUNT'] = len(TALL_CHARS)
    layout = ['/* Generated by tools/assets.py. Do not edit. */']
    layout.append(f'#define TALL_CHARS "{TALL_CHARS}"')
    for k, v in defs.items():
        layout.append(f'#define {k} {v}')
    for k, v in init.items():
        layout.append(f'#define {k}_INIT {{' + ','.join(str(x) for x in v) + '}')
    Path(out).parent.joinpath('layout.h').write_text('\n'.join(layout) + '\n')
    Path(out).parent.joinpath('colors.h').write_text('\n'.join(colors) + '\n')
    Path(out).write_text('\n'.join(h) + '\n')
    print(f'assets: {len(obj0)} sprite, {len(obj1)} popup, {len(font)} font, {len(hud)} HUD, '
          f'{len(base_tiles)} base, {len(title_tiles)} title, {len(splash_tiles)} splash tiles')
    if preview:
        render_preview(preview, btiles, battrs, base_ts, quads, tmap, tattr, title_ts)


# --------------------------------------------------------------- preview
def render_preview(path, btiles, battrs, base_ts, quads, tmap, tattr, title_ts):
    """A contact sheet for checking the art without building the ROM."""
    from PIL import Image
    S = 4
    sheet = Image.new('RGB', (160 * 2 + 24, 300), tuple(v * 8 for v in INK))
    px = sheet.load()

    def draw(p, x0, y0, pal, scale=1):
        for y, row in enumerate(p):
            for x, v in enumerate(row):
                if v:
                    c = pal[v]
                    for a in range(scale):
                        for b in range(scale):
                            px[x0 + x * scale + a, y0 + y * scale + b] = tuple(min(255, k * 8 + k // 4) for k in c)
    for f in range(9):
        for fr in range(2):
            draw(invader(FORM_TILES[f], fr), 4 + f * 36 + fr * 17, 4, FORMS[f])
    for i in range(8):
        draw(logo(i), 4 + i * 20, 26, LOGO_PALS[i])
        draw(logo(i), 4 + i * 40, 46, LOGO_PALS[i], 2)
    for i, ch in enumerate('DREAMBASE INVADERS 0123456789!'):
        draw(font_tile(ch), 4 + i * 8, 82, TEXT)
    draw(parse(MARK48), 4, 96, [INK, DREAM_DARK, DREAM_MID, DREAM])
    draw(parse(WORD14), 60, 112, TEXT)
    draw(invaders_word(), 4, 150, [INK, COMPANIES[2][1], (3, 3, 8), WHITE])
    pm, pw = wordmark_row()
    draw(pm, 170, 150, GREEN_TEXT)
    draw(pw, 196, 150, TEXT)
    pals = {3: CANNON_FLASH, 4: STARS, 5: MOON, 6: BASE, 7: cannon_pal(0)}
    lookup = {}
    for bank, index, data in base_ts.tiles:
        lookup[index] = data

    def tile_pixels(data, attr):
        rows = []
        for y in range(8):
            lo, hi = data[y * 2], data[y * 2 + 1]
            rows.append([((lo >> (7 - x)) & 1) | (((hi >> (7 - x)) & 1) << 1) for x in range(8)])
        if attr & 0x20:
            rows = [r[::-1] for r in rows]
        if attr & 0x40:
            rows = rows[::-1]
        return rows
    for ty in range(4):
        for tx in range(20):
            t, a = btiles[ty][tx], battrs[ty][tx]
            if t in lookup:
                draw(tile_pixels(lookup[t], a), 4 + tx * 8, 190 + ty * 8, pals[a & 7])
    for slot in range(5):
        if slot == 2:
            continue
        quad = quads['cannon']
        for k, (t, a) in enumerate(quad):
            draw(tile_pixels(lookup[t], a), 4 + SLOTS[slot] - 8 + (k & 1) * 8, 190 + 8 + (k >> 1) * 8, cannon_pal(slot) if slot else CANNON_FLASH)
    draw(hatch_picture(), 180, 190, cannon_pal(1))
    for i in range(8):
        draw(mini_logo(i), 4 + i * 12, 240, [INK, (4, 4, 4), COMPANIES[i][2], COMPANIES[i][1]])
    for i, ch in enumerate(POP_CHARS):
        draw(pop_tile(ch), 110 + i * 6, 240, GOLD_TEXT)
    for k in range(8):
        draw(spark(k), 4 + k * 10, 256, [INK, (10, 10, 10), (20, 20, 20), WHITE])
    for f in range(9):
        draw(bar_tile(f), 100 + f * 9, 256, [INK, (8, 8, 10), (8, 20, 12), (13, 30, 21)])
    draw(icon_heart(), 190, 256, [INK, (8, 0, 0), DANGER, WHITE])
    draw(icon_life(), 200, 256, TEXT)
    sheet = sheet.resize((sheet.width * S // 2, sheet.height * S // 2), Image.NEAREST)
    sheet.save(path)


if __name__ == '__main__':
    main(*sys.argv[1:])
