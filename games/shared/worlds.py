"""Native pixel art for Stormkite, Comet Links and Prism Well.

assets.py builds the tiles every cartridge shares (font, common patterns, the
planet). This module adds each new game's own background tiles, scenery and
sprites. Background slots 76-95, 96-159 and 176-191 are free in these three
cartridges because they do not use Bloom Circuit's pipe tiles.
"""

FREE_SLOTS = list(range(76, 96)) + list(range(96, 160)) + list(range(176, 192))


def canvas(w, h):
    return [[0] * w for _ in range(h)]


def grid(rows):
    return [[int(c) for c in row] for row in rows]


def encode(p):
    out = []
    for row in p:
        lo = hi = 0
        for v in row:
            lo = (lo << 1) | (v & 1)
            hi = (hi << 1) | ((v >> 1) & 1)
        out.extend((lo, hi))
    return out


def put(p, x, y, c, wrap=False):
    if wrap:
        x %= len(p[0])
    if 0 <= y < len(p) and 0 <= x < len(p[0]):
        p[y][x] = c


def disc(p, cx, cy, r, c, wrap=False):
    for y in range(int(cy - r - 1), int(cy + r + 2)):
        for x in range(int(cx - r - 1), int(cx + r + 2)):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                put(p, x, y, c, wrap)


def rect(p, x0, y0, x1, y1, c, wrap=False):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put(p, x, y, c, wrap)


def block(p, x, y):
    return [row[x:x + 8] for row in p[y:y + 8]]


class Tiles:
    """Deduplicates 8x8 blocks into free background slots, reusing flips."""

    def __init__(self, bg, slots=FREE_SLOTS):
        self.bg = bg
        self.slots = list(slots)
        self.known = {tuple(encode(canvas(8, 8))): (0, 0)}

    def add(self, b):
        key = tuple(encode(b))
        if key in self.known:
            return self.known[key]
        assert self.slots, 'background tile budget exhausted'
        slot = self.slots.pop(0)
        self.bg[slot] = list(key)
        flips = {
            0x20: [row[::-1] for row in b],
            0x40: b[::-1],
            0x60: [row[::-1] for row in b[::-1]],
        }
        self.known[key] = (slot, 0)
        for attr, variant in flips.items():
            self.known.setdefault(tuple(encode(variant)), (slot, attr))
        return slot, 0

    def paint(self, p, pal, scene, colors, row0):
        """Store canvas p at map row row0; pal(cx, cy) chooses each palette."""
        for cy in range(len(p) // 8):
            for cx in range(len(p[0]) // 8):
                slot, attr = self.add(block(p, cx * 8, cy * 8))
                n = (row0 + cy) * 32 + cx
                scene[n] = slot
                colors[n] = pal(cx, cy) | attr


# --------------------------------------------------------------- Stormkite
# World rows 2-16 form three parallax bands. The first pixel row of every
# band stays empty so a one-line-late scroll split cannot be seen.

def stormkite_world(bg):
    scene, colors = [0] * 1024, [0] * 1024
    tiles = Tiles(bg)
    far = canvas(256, 40)
    for x, y in [(3, 1), (11, 3), (17, 0), (29, 2), (6, 4), (20, 1), (14, 2)]:
        for rep in range(0, 256, 128):
            put(far, rep + x * 8 + 3, y * 8 + 3, 3)
    for x, y in [(1, 0), (9, 1), (22, 0), (27, 3), (13, 4)]:
        for dx, dy, c in [(0, 0, 3), (-1, 0, 2), (1, 0, 2), (0, -1, 2), (0, 1, 2)]:
            put(far, x * 8 + 4 + dx, y * 8 + 4 + dy, c)
    disc(far, 212, 13, 10, 3)
    disc(far, 216, 10, 7.5, 0)
    disc(far, 216, 10, 5, 0)
    # Clouds are capsules with puffs on cell centres, so their middles repeat.
    for x0, cells, y0 in [(16, 7, 17), (104, 5, 25), (160, 4, 17)]:
        x1 = x0 + cells * 8 - 1
        for y in range(y0, y0 + 10):
            for x in range(x0, x1 + 1):
                inside = (x0 + 5 <= x <= x1 - 5) or ((min(abs(x - x0 - 5), abs(x - x1 + 5))) ** 2 + (y - y0 - 5) ** 2 <= 25)
                if inside:
                    far[y][x] = 1 if y > y0 + 6 else 2
        for k in range(1, cells - 1):
            cx = x0 + k * 8 + 4
            for y in range(y0 - 5, y0 + 3):
                for x in range(cx - 5, cx + 6):
                    if (x - cx) ** 2 + (y - y0 - 1) ** 2 <= 22 and far[y][x] == 0:
                        far[y][x] = 3 if y < y0 - 2 else 2
    for y in range(33, 40):
        for x in range(256):
            if y >= 37 or (x + y) % 2 == 0:
                far[y][x] = 1 if y < 39 else (1 if x % 2 else 0)
    tiles.paint(far, lambda cx, cy: 5, scene, colors, 2)

    mid = canvas(256, 40)
    heights = [2, 2, 3, 3, 1, 4, 4, 2, 1, 3, 3, 3, 2, 1, 1, 4, 4, 4, 2, 2, 3, 1, 2, 2, 4, 3, 3, 1, 2, 3, 3, 2]
    windows = [[(1, 3, 4, 3)], [(1, 3, 4, 3), (4, 7, 7, 3)], [(1, 7, 7, 3)], [(4, 3, 7, 2)]]
    for col, h in enumerate(heights):
        top = 40 - h * 8
        rect(mid, col * 8, top + 1, col * 8 + 7, 39, 1)
        for row in range(h):
            for x, y, _, c in windows[(col * 5 + row * 3) % 4]:
                rect(mid, col * 8 + x, top + row * 8 + y, col * 8 + x + 1, top + row * 8 + y + 1, c)
        if h >= 3 and col % 3 == 0:
            for y in range(top - 6, top + 1):
                put(mid, col * 8 + 3, y, 2)
            put(mid, col * 8 + 3, top - 7, 3)
    # Two pagoda roofs with upswept eaves.
    for col in (5, 16):
        top = 40 - heights[col] * 8
        for i in range(12):
            y = top - 1 - i // 2
            for x in range(col * 8 - 4 + i, col * 8 + 20 - i):
                if 0 < y:
                    put(mid, x, y, 2 if i < 2 else 1)
        put(mid, col * 8 - 5, top - 2, 2)
        put(mid, col * 8 + 20, top - 2, 2)
    for x in range(256):
        mid[0][x] = 0
    tiles.paint(mid, lambda cx, cy: 7, scene, colors, 7)

    near = canvas(256, 40)
    roofs = [(0, 9, 12), (80, 7, 20), (144, 9, 12), (224, 4, 20)]
    for x0, cells, peak in roofs:
        width = cells * 8
        for y in range(peak, 32):
            inset = max(0, (peak + 8 - y)) * 2
            for x in range(x0 + inset - 2, x0 + width - inset + 2):
                if y < peak + 8:
                    c = 2 if (y - peak) % 4 == 0 else (1 if (x // 4 + y // 4) % 2 else 2)
                else:
                    c = 1
                put(near, x, y, c, True)
        for wx in range(x0 + 10, x0 + width - 8, 16):
            rect(near, wx, 26, wx + 3, 29, 3, True)
            rect(near, wx + 1, 26, wx + 2, 29, 2, True)
    for x in range(256):
        sag = (1 if 3 <= x % 16 <= 12 else 0) + (1 if 6 <= x % 16 <= 9 else 0)
        if near[3 + sag][x] == 0:
            put(near, x, 3 + sag, 1)
        if x % 16 == 0:
            rect(near, x - 1, 5, x + 1, 8, 3, True)
            put(near, x, 4, 1, True)
            put(near, x - 1, 8, 2, True)
            put(near, x + 1, 8, 2, True)
    for y in range(32, 40):
        for x in range(256):
            wave = (x + (y - 32) * 3) % 16
            near[y][x] = 2 if (y == 33 and wave < 4) or (y == 36 and 8 <= wave < 11) else (1 if y >= 34 else 0)
    for x in range(256):
        near[0][x] = 0
    tiles.paint(near, lambda cx, cy: 2 if cy == 4 else 6, scene, colors, 12)
    return scene, colors, len(FREE_SLOTS) - len(tiles.slots)


STORMKITE_HERO = grid([
    '0000000330000000',
    '0000003223000000',
    '0000032222300000',
    '0000322332230000',
    '0003222332223000',
    '0032222332222300',
    '0322222332222230',
    '3333333333333333',
    '0311111331111130',
    '0031111331111300',
    '0003111331113000',
    '0000311331130000',
    '0000031331300000',
    '0000003333000000',
    '0000000330000000',
    '0000000110000000',
])

STORMKITE_FOE = grid([
    '0000011111000000',
    '0001122222110000',
    '0012222222221100',
    '0122222222222210',
    '1222222222222221',
    '1223322222332221',
    '1223332223332221',
    '1222222222222221',
    '1122223333222211',
    '0112222222222110',
    '0011111111111100',
    '0000030000300000',
    '0000033003300000',
    '0000003003000000',
    '0000000330000000',
    '0000000000000000',
])

STORMKITE_SPRITES = [
    # 16 paper dart
    ['00000000', '00000000', '11100000', '02333300', '02333333', '11100000', '00000000', '00000000'],
    # 17-18 wisp, two animation frames
    ['00011000', '00122100', '01233210', '12322321', '12233221', '01222210', '00100100', '01000010'],
    ['00011000', '00122100', '01233210', '12322321', '12233221', '01222210', '00011000', '00100100'],
    # 19 enemy bolt
    ['00000000', '00003300', '00033000', '00333300', '00003300', '00033000', '00030000', '00000000'],
    # 20 festival lantern
    ['00011000', '00333300', '03233230', '03333330', '03233230', '00333300', '00011000', '00001000'],
    # 21 kite tail bow
    ['00000000', '03000030', '02300320', '02233220', '02300320', '03000030', '00000000', '00000000'],
    # 22-23 swooping storm swift
    ['00000000', '30000003', '23000032', '02333320', '00211200', '00022000', '00000000', '00000000'],
    ['00000000', '00000000', '00000000', '33311333', '02333320', '00022000', '00000000', '00000000'],
    # 24 gust ring
    ['00333300', '03000030', '30000003', '30000003', '30000003', '30000003', '03000030', '00333300'],
    # 25 pop burst
    ['30003000', '03030300', '00303000', '33030330', '00303000', '03030300', '30003000', '00000000'],
]

# The Thunderhead is 24 x 24, assembled from nine sprites.
THUNDERHEAD = grid([
    '000000011111111000000000',
    '000001122222222110000000',
    '000112222222222221100000',
    '001222222222222222211000',
    '012222222222222222222100',
    '122222222222222222222210',
    '122223332222222333222221',
    '122233333222223333322221',
    '122233113222223113322221',
    '122223332222222333222221',
    '122222222222222222222221',
    '112222222222222222222211',
    '122222233333333332222221',
    '122222331313131313222221',
    '112222233333333332222211',
    '011222222222222222222110',
    '001111222222222222111100',
    '000001111111111111100000',
    '000000003000000300000000',
    '000000033000000330000000',
    '000000330000003300000000',
    '000000030000000300000000',
    '000000003000000030000000',
    '000000000000000000000000',
])


def tiles16(p, cols, rows):
    """Row-major 8x8 sprite tiles from a larger drawing."""
    return [encode(block(p, x * 8, y * 8)) for y in range(rows) for x in range(cols)]


def build(kind, bg):
    if kind == 5:
        scene, colors, used = stormkite_world(bg)
        sprites = [encode(grid(t)) for t in STORMKITE_SPRITES] + tiles16(THUNDERHEAD, 3, 3)
        return {'hero': STORMKITE_HERO, 'foe': STORMKITE_FOE, 'sprites': sprites,
                'scene': scene, 'colors': colors}
    if kind == 6:
        scene, colors = comet_world(bg)
        return {'hero': COMET_HERO, 'foe': COMET_FOE, 'scene': scene, 'colors': colors,
                'sprites': [encode(grid(t)) for t in COMET_SPRITES]}
    if kind == 7:
        scene, colors = prism_world(bg)
        sprites = [encode(grid(t)) for t in PRISM_GEMS + [PRISM_ART['stone'], PRISM_ART['prism'], PRISM_ART['flash']]]
        return {'hero': PRISM_HERO, 'foe': PRISM_FOE, 'scene': scene, 'colors': colors,
                'sprites': sprites}
    raise ValueError(kind)


# ------------------------------------------------------------- Comet Links
COMET_TILES = {
    'rock': [76, 77, 78], 'nebula': [79, 80], 'bumper': 81, 'cup': 82,
    'void': 83, 'wind': {'>': 84, '<': 85, '^': 86, 'v': 87},
    'moon': [88, 89, 90, 91], 'pip_half': 92, 'tee': 93, 'pip_full': 94,
    'pip_empty': 95, 'dust': 176,
}
COMET_ART = {
    'rock': [
        ['01122110', '12222221', '12232221', '12222211', '11222221', '12221221', '12222221', '01111110'],
        ['01111110', '12223221', '12222221', '11221221', '12222221', '12222111', '12211221', '01111110'],
        ['01122210', '12222221', '12112221', '12222321', '12222221', '11222221', '12222211', '01111110'],
    ],
    'nebula': [
        ['10001000', '01010100', '00100010', '01010101', '10001000', '01000101', '00100010', '01010100'],
        ['00100010', '01010101', '10001000', '01000101', '00100010', '01010100', '10001000', '01010100'],
    ],
    'bumper': ['00333300', '03222230', '32211223', '32100123', '32100123', '32211223', '03222230', '00333300'],
    'cup': ['00022000', '02211220', '21100112', '21000012', '21000012', '21100112', '02211220', '00022000'],
    'void': ['00111100', '01222210', '12233221', '12300321', '12300321', '12233221', '01222210', '00111100'],
    'wind': {
        '>': ['00000000', '01100000', '00110000', '00011000', '00110000', '01100000', '00000000', '00000000'],
        '<': ['00000000', '00000110', '00001100', '00011000', '00001100', '00000110', '00000000', '00000000'],
        '^': ['00000000', '00000000', '00010000', '00111000', '01101100', '11000110', '00000000', '00000000'],
        'v': ['00000000', '00000000', '11000110', '01101100', '00111000', '00010000', '00000000', '00000000'],
    },
    'pip_half': ['00000000', '33330000', '32220000', '32220000', '32220000', '32220000', '11110000', '00000000'],
    'pip_full': ['00000000', '33333330', '32222220', '32222220', '32222220', '32222220', '11111110', '00000000'],
    'pip_empty': ['00000000', '11111110', '10000010', '10000010', '10000010', '10000010', '11111110', '00000000'],
    'tee': ['00000000', '00000000', '00010000', '00121000', '00010000', '00000000', '00000000', '00000000'],
    'dust': ['00000000', '00000000', '00000000', '00000000', '00000200', '00000000', '00000000', '00000000'],
}
COMET_HERO = grid([
    '0000000000000000',
    '0000000000000000',
    '0000000000000110',
    '0000000000011221',
    '0000000001122331',
    '0000000112233331',
    '0000011223333310',
    '0001122333333100',
    '0112223333331000',
    '0000112233310000',
    '0000001122100000',
    '0000000011000000',
    '0000000000000000',
    '0000000000000000',
    '0000000000000000',
    '0000000000000000',
])
COMET_FOE = grid([
    '0000000330000000',
    '0000003223000000',
    '0000032222300000',
    '0000322222230000',
    '0003222222223000',
    '3332222222222333',
    '0322222222222230',
    '0032211111122300',
    '0003211111123000',
    '0000321111230000',
    '0000032222300000',
    '0000003223000000',
    '0000000330000000',
    '0000000000000000',
    '0000000000000000',
    '0000000000000000',
])
COMET_SPRITES = [
    # 16 comet ball
    ['00000000', '00000000', '00033000', '00333300', '00333300', '00033000', '00000000', '00000000'],
    # 17-18 comet trail
    ['00000000', '00000000', '00000000', '00022000', '00022000', '00000000', '00000000', '00000000'],
    ['00000000', '00000000', '00000000', '00010000', '00000000', '00000000', '00000000', '00000000'],
    # 19 aim dot, 20 scope dot
    ['00000000', '00000000', '00000000', '00033000', '00033000', '00000000', '00000000', '00000000'],
    ['00000000', '00000000', '00022000', '00233200', '00233200', '00022000', '00000000', '00000000'],
    # 21-22 cup pennant
    ['03300000', '03333000', '03333300', '03330000', '03000000', '03000000', '03000000', '03000000'],
    ['03000000', '03330000', '03333300', '03333000', '03300000', '03000000', '03000000', '03000000'],
    # 23-24 black hole swirl
    ['00022000', '00200000', '02003300', '20030020', '02003002', '00330020', '00000200', '00022000'],
    ['00022000', '00000200', '00330020', '02003002', '20030020', '02003300', '00200000', '00022000'],
    # 25 sparkle
    ['30000003', '03000030', '00300300', '00033000', '00033000', '00300300', '03000030', '30000003'],
]


def comet_world(bg):
    art = COMET_ART
    slots = COMET_TILES
    for i, rows in enumerate(art['rock']):
        bg[slots['rock'][i]] = encode(grid(rows))
    for i, rows in enumerate(art['nebula']):
        bg[slots['nebula'][i]] = encode(grid(rows))
    for key in ('bumper', 'cup', 'void', 'pip_half', 'pip_full', 'pip_empty', 'tee', 'dust'):
        bg[slots[key]] = encode(grid(art[key]))
    for key, rows in art['wind'].items():
        bg[slots['wind'][key]] = encode(grid(rows))
    moon = canvas(16, 16)
    for y in range(16):
        for x in range(16):
            d = (x - 7.5) ** 2 + (y - 7.5) ** 2
            if d < 56:
                moon[y][x] = 1 if x + y > 19 else (3 if (x - 5) ** 2 + (y - 5) ** 2 < 6 else 2)
    for x, y in [(9, 5), (10, 5), (5, 10), (6, 10), (6, 9)]:
        moon[y][x] = 1
    for i, (yy, xx) in enumerate([(0, 0), (0, 8), (8, 0), (8, 8)]):
        bg[slots['moon'][i]] = encode(block(moon, xx, yy))
    scene, colors = [0] * 1024, [0] * 1024
    for y in range(2, 17):
        for x in range(20):
            h = (x * 29 + y * 47) % 37
            if h == 0:
                scene[y * 32 + x] = 63
            elif h in (5, 11, 19):
                scene[y * 32 + x] = slots['dust']
            colors[y * 32 + x] = 4
    return scene, colors



# -------------------------------------------------------------- Prism Well
# Every colour also has its own silhouette: diamond, orb, triangle, square
# and cross. Gems fill a 7 x 7 box so neighbours keep a one-pixel gutter.
PRISM_TILES = {'gems': [76, 77, 78, 79, 80], 'stone': 81, 'prism': 82, 'flash': 83,
               'ghost': 84, 'wall': 85, 'rim': 86, 'corner': 87, 'dust': 88,
               'edge_h': 89, 'edge_v': 90}
PRISM_GEMS = [
    ['00010000', '00131000', '01332100', '13322210', '01222100', '00121000', '00010000', '00000000'],
    ['00111000', '01332100', '13322210', '13222210', '12222210', '01222100', '00111000', '00000000'],
    ['00010000', '00131000', '00131000', '01322100', '01222100', '12222210', '11111110', '00000000'],
    ['11111110', '13332210', '13222210', '13222210', '12222210', '12222210', '11111110', '00000000'],
    ['00111000', '00131000', '11131110', '13322210', '11222110', '00121000', '00111000', '00000000'],
]
PRISM_ART = {
    'stone': ['33333310', '32222210', '32122210', '32221210', '32222210', '32212210', '11111110', '00000000'],
    'prism': ['00131000', '01333100', '13323310', '13232310', '13323310', '01333100', '00131000', '00000000'],
    'flash': ['10030010', '01030100', '00333000', '33333330', '00333000', '01030100', '10030010', '00000000'],
    'ghost': ['10101010', '00000000', '10000010', '00000000', '10000010', '00000000', '10101010', '00000000'],
    'wall': ['21122123', '21122123', '11111113', '22112213', '22112213', '11111113', '21122123', '21122123'],
    'rim': ['33333333', '22222222', '11111111', '00000000', '00000000', '00000000', '00000000', '00000000'],
    'corner': ['00000000', '00000000', '00002222', '00021111', '00210000', '00210000', '00210000', '00210000'],
    'edge_h': ['00000000', '00000000', '22222222', '11111111', '00000000', '00000000', '00000000', '00000000'],
    'edge_v': ['00210000', '00210000', '00210000', '00210000', '00210000', '00210000', '00210000', '00210000'],
    'dust': ['00000000', '00000000', '00000000', '00000000', '00001000', '00000000', '00000000', '00000000'],
}
PRISM_HERO = grid([
    '0000000330000000',
    '0000003333000000',
    '0000033223300000',
    '0000332222330000',
    '0003322222233000',
    '0033222222223300',
    '0332222112222330',
    '0322221111222230',
    '0322211111122230',
    '0332221111222330',
    '0033222112223300',
    '0003322222233000',
    '0000332222330000',
    '0000033223300000',
    '0000003333000000',
    '0000000330000000',
])
PRISM_FOE = grid([
    '0000000000000000',
    '0033333333333100',
    '0322222222222100',
    '0322122222212100',
    '0322222222222100',
    '0322222112222100',
    '0322221111222100',
    '0321222112222100',
    '0322222222212100',
    '0322222222222100',
    '0322212222222100',
    '0322222222222100',
    '0111111111111100',
    '0000000000000000',
    '0000000000000000',
    '0000000000000000',
])


def prism_world(bg):
    slots = PRISM_TILES
    for slot, rows in zip(slots['gems'], PRISM_GEMS):
        bg[slot] = encode(grid(rows))
    for key, rows in PRISM_ART.items():
        bg[slots[key]] = encode(grid(rows))
    scene, colors = [0] * 1024, [0] * 1024
    for y in range(2, 17):
        for x in range(20):
            n = y * 32 + x
            if x in (1, 9):
                scene[n] = slots['wall']
                colors[n] = 4 | (0x20 if x == 9 else 0)
            elif x > 10 and (x * 13 + y * 7) % 23 == 0:
                scene[n] = slots['dust']
                colors[n] = 4
    for x in range(1, 10):
        scene[17 * 32 + x] = 0
    # Panel frames: NEXT (rows 3-8) and GOALS (rows 10-16).
    for top, bottom in ((3, 8), (10, 16)):
        for y in range(top, bottom + 1):
            for x in range(11, 20):
                n = y * 32 + x
                flip = (0x20 if x == 19 else 0) | (0x40 if y == bottom else 0)
                if x in (11, 19) and y in (top, bottom):
                    scene[n] = slots['corner']
                elif y in (top, bottom):
                    scene[n] = slots['edge_h']
                elif x in (11, 19):
                    scene[n] = slots['edge_v']
                else:
                    scene[n] = 0
                    continue
                colors[n] = 4 | flip
    return scene, colors
