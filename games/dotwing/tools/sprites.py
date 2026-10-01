"""Native sprite art. '.' is transparent; 1, 2 and 3 are palette colours.

All sprites use 8x16 hardware objects. Small 8x8 subjects sit in the top half
of their object; the bottom half is blank so their hitboxes stay simple.
"""
from gbgfx import Pic

KEY = {'1': 1, '2': 2, '3': 3}


def art(*rows):
    return Pic.ascii(rows, KEY)


def pad16(p, w=None):
    """Grow a Pic to a multiple of 8 wide and 16 tall (top-left anchored)."""
    w = w or ((p.w + 7) // 8 * 8)
    q = Pic(w, (p.h + 15) // 16 * 16)
    q.blit(p, 0, 0)
    return q


# ----------------------------------------------------------------- player
# Ivory airframe (3), livery panels (2) and ink outline (1). The canopy is
# a dark bubble with your Dot's colour glowing inside.
def mirrored(*left):
    """Author the left eight columns; the right half mirrors them."""
    return art(*[row + row[::-1] for row in left])


PLANE = mirrored(
    '.......1',
    '......13',
    '......13',
    '.....133',
    '.....132',
    '.....122',
    '....1312',
    '...13311',
    '..133333',
    '.1333333',
    '12233332',
    '12223332',
    '.1111333',
    '....1333',
    '....1221',
    '.....11.',
)
# A glint on the canopy glass.
PLANE.put(6, 4, 3)
PLANE.put(7, 4, 1)


def banked(p):
    """Roll the plane left: the raised left wing foreshortens by two pixels
    and the fuselage slips one pixel towards the turn."""
    q = Pic(16, 16)
    keep = [0, 1, 3, 5, 6, 7]
    for y in range(16):
        for i, x in enumerate(keep):
            q.px[y][i + 1] = p.px[y][x]
        for x in range(8, 16):
            q.px[y][x - 1] = p.px[y][x]
    for y in range(9, 12):
        # The lowered right wing shows its livery underside.
        for x in range(9, 14):
            if q.px[y][x] == 3 and q.px[y + 1][x] in (1, 2):
                q.px[y][x] = 2
    return q


PLANE_BANK = banked(PLANE)
# Engine flames, fire palette: two flicker frames under the tail.
FLAME = [art('.3..3...', '.32.32..', '.22.22..', '..1..1..'),
         art('.3..3...', '.3..3...', '.32.32..', '.22.22..', '.1..1...')]
# Soft shadow cast on the scenery far below (shadow palette colour 1).
SHADOW = art(
    '...11...',
    '...11...',
    '..1111..',
    '.111111.',
    '11111111',
    '1.1111.1',
    '...11...',
    '..1111..',
)

# Twin laser bolt (energy palette) and its angled variants.
SHOT = art('.3...3..', '.3...3..', '232.232.', '232.232.', '232.232.', '.2...2..', '.1...1..', '.1...1..')
SHOT_L = art('3.......', '33......', '.32.....', '.232....', '..22....', '...21...', '....1...')
SHOT_R = SHOT_L.flip_h()
SHOT_SOLO = art('...3....', '..232...', '..232...', '..232...', '..232...', '...2....', '...1....', '...1....')

# Ally dot: a tiny winged Dot in your livery.
ALLY = [art('...11...', '..1331..', '1.1321.1', '1312213.', '.112211.', '..1221..', '...11...'),
        art('...11...', '..1331..', '..1321..', '13122131', '.112211.', '..1221..', '...11...')]
# Focus core: the precise hitbox shown while holding A.
CORE = art('..11....', '.1331...', '.1331...', '..11....')

# ---------------------------------------------------------------- bullets
BULLET_SMALL = art('........', '..111...', '.13321..', '.13221..', '.12221..', '..111...')
BULLET_BIG = art('..111...', '.13321..', '1333221.', '1332221.', '1322221.', '.12221..', '..111...')
BULLET_NEEDLE = art('...11...', '..1331..', '..1321..', '..1321..', '..1221..', '..1221..', '...11...')
BULLET_STAR = art('...1....', '..131...', '.13321..', '1332221.', '.12221..', '..121...', '...1....')

# ---------------------------------------------------------------- pickups
COIN = [
    art('..1111..', '.122221.', '12233221', '12232221', '12232221', '12222221', '.122221.', '..1111..'),
    art('...111..', '..12221.', '.1233221', '.1232221', '.1232221', '.1222221', '..12221.', '...111..'),
    art('...11...', '...131..', '...131..', '...121..', '...121..', '...121..', '...131..', '...11...'),
    art('..111...', '.12221..', '1223321.', '1223221.', '1223221.', '1222221.', '.12221..', '..111...'),
]
POWER = art('..1111..', '.133331.', '13111331', '13131321', '13111321', '13133221', '.122221.', '..1111..')
REPAIR = art('...11...', '..1331..', '1113311.', '13333331', '12223221', '1112211.', '..1221..', '...11...')
BOLT = art('....111.', '...1331.', '..1331..', '.133331.', '..1331..', '..131...', '.131....', '.11.....')

# ------------------------------------------------------------- explosions
def _blob(puffs, size=16):
    """Union of shaded puffs: bright cores, orange bodies, dark red rims."""
    p = Pic(size, size)
    for cx, cy, r, core in puffs:
        for y in range(size):
            for x in range(size):
                d = ((x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2) ** 0.5
                if d <= r:
                    v = 3 if d <= r * core else (2 if d <= r - 1.2 else 1)
                    old = p.px[y][x]
                    p.px[y][x] = v if old is None else max(old, v)
    return p


EXPLOSION = [
    _blob([(8, 8, 3.2, 0.75)]),
    _blob([(8, 8, 5.4, 0.55), (4.5, 6, 2.6, 0.3), (11.5, 6.5, 2.8, 0.3), (8, 12, 2.6, 0.2)]),
    _blob([(8, 8, 6.4, 0.4), (3.5, 5, 3.2, 0.2), (12.5, 5.5, 3.2, 0.2), (4, 12, 3, 0.1), (12, 12, 3.2, 0.1)]),
    _blob([(4, 5, 3.6, 0.15), (12, 4.5, 3.4, 0.15), (3.5, 12, 3.2, 0), (12.5, 12, 3.6, 0.1), (8, 8.5, 3.2, 0.35)]),
    _blob([(3, 4, 2.6, 0), (13, 3.5, 2.4, 0), (2.5, 13, 2.2, 0), (13.5, 13, 2.6, 0), (8, 9, 1.8, 0)]),
    _blob([(2.5, 3, 1.5, 0), (13.5, 2.5, 1.4, 0), (2, 14, 1.3, 0), (14, 14, 1.5, 0)]),
]
SPARK = [art('...3....', '...3....', '..323...', '3322233.', '..323...', '...3....', '...3....'),
         art('3.....3.', '.3...3..', '..323...', '...2....', '..323...', '.3...3..', '3.....3.')]
RING = art('..1111..', '.1....1.', '1......1', '1......1', '1......1', '1......1', '.1....1.', '..1111..')
SPEEDLINE = art('...1....', '...2....', '...2....', '...3....', '...3....', '...3....', '...3....', '...2....',
                '...2....', '...1....')
