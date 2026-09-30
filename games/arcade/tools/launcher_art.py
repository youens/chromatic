"""Native art and text tables for the Chromatic Arcade launcher.

Writes two headers into games/arcade/build:
  launcher-art.h   tile data, compiled into its own ROM bank
  launcher-data.h  maps, palettes and text, kept in the fixed bank

Each game has a 48 x 48 cartridge-label icon drawn with four colours. Colour
0 is the launcher's ink so the rounded card corners blend into the shelf.
Icon tiles are deduplicated (including flips) and packed into VRAM bank 1
first, then into free bank 0 slots.
"""
from pathlib import Path
import ast
import math
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'games/arcade/build'
FONT = ast.literal_eval(re.search(r'FONT = (\{.*?\n\})', (ROOT / 'games/shared/assets.py').read_text(), re.S).group(1))
INK = (2, 2, 6)

# Launcher order. kind is a runtime-game index for the dispatch table, or
# 'hello' for Hello Dot, which keeps its own engine.
GAMES = [
    {'slug': 'neon-wake', 'kind': 0, 'name': 'NEON WAKE', 'genre': 'ARCADE RACING',
     'tagline': 'OUTRUN THE NIGHT', 'blurb': ['THREAD THE TRAFFIC', 'ON A BENDING ROAD'],
     'palette': [INK, (9, 5, 19), (9, 31, 31), (30, 6, 23)]},
    {'slug': 'moonthread', 'kind': 1, 'name': 'MOONTHREAD', 'genre': 'GRAVITY PLATFORMER',
     'tagline': 'GRAVITY IS A THREAD', 'blurb': ['FLIP GRAVITY IN', 'SIX LUNAR ROOMS'],
     'palette': [INK, (8, 7, 20), (29, 20, 8), (30, 29, 27)]},
    {'slug': 'echo-vault', 'kind': 2, 'name': 'ECHO VAULT', 'genre': 'SONAR STEALTH',
     'tagline': 'EVERY ECHO TELLS', 'blurb': ['FIND THREE KEYS', 'WHILE GUARDS LISTEN'],
     'palette': [INK, (3, 11, 12), (8, 25, 20), (31, 27, 12)]},
    {'slug': 'bloom-circuit', 'kind': 3, 'name': 'BLOOM CIRCUIT', 'genre': 'LIVING PUZZLE',
     'tagline': 'WAKE THE GARDEN', 'blurb': ['TURN ROOTS UNTIL', 'EVERY TILE GLOWS'],
     'palette': [INK, (5, 13, 9), (14, 28, 14), (30, 16, 22)]},
    {'slug': 'orbit-choir', 'kind': 4, 'name': 'ORBIT CHOIR', 'genre': 'RHYTHM DEFENSE',
     'tagline': 'MAKE THE STARS SING', 'blurb': ['STRIKE EACH NOTE', 'ON THE ORBIT RING'],
     'palette': [INK, (6, 9, 22), (9, 18, 30), (31, 24, 18)]},
    {'slug': 'hello-dot', 'kind': 'hello', 'name': 'HELLO DOT', 'genre': 'SPARK CHASER',
     'tagline': 'SMALL DOT BIG HELLO', 'blurb': ['CHAIN SPARKS AND', 'DASH THROUGH BUGS'],
     'palette': [INK, (12, 5, 14), (13, 29, 22), (31, 26, 6)],
     'controls': ['D PAD MOVE', 'A DASH  B SLOW', '5 SPARKS = HELLO']},
    {'slug': 'stormkite', 'kind': 5, 'name': 'STORMKITE', 'genre': 'PARALLAX SHOOTER',
     'tagline': 'RIDE THE THUNDER', 'blurb': ['FLY A PAPER KITE', 'INTO THE STORM'],
     'palette': [INK, (10, 6, 20), (21, 15, 31), (31, 13, 10)]},
    {'slug': 'comet-links', 'kind': 6, 'name': 'COMET LINKS', 'genre': 'GRAVITY MINI GOLF',
     'tagline': 'EVERY PLANET PULLS', 'blurb': ['PUTT A COMET PAST', 'NINE HUNGRY PLANETS'],
     'palette': [INK, (4, 8, 20), (10, 24, 31), (31, 31, 31)]},
    {'slug': 'prism-well', 'kind': 7, 'name': 'PRISM WELL', 'genre': 'CRYSTAL PUZZLE',
     'tagline': 'LINE UP THE LIGHT', 'blurb': ['MATCH THREE COLORS', 'TO BREAK THE STONE'],
     'palette': [INK, (11, 5, 17), (8, 24, 30), (31, 13, 22)]},
    {'slug':'dot-swarm','kind':8,'name':'DOT SWARM','genre':'NEON SNAKE ARENA',
     'tagline':'GLOW GROW GO','blurb':['GROW YOUR DOT CHAIN','OWN TRAIL IS SAFE'],
     'controls':['D PAD STEER','A BOOST','GROW TO 48 DOTS'],
     'palette':[INK,(1,10,15),(0,27,29),(31,8,22)]},
]


def canvas(w, h, c=0):
    return [[c] * w for _ in range(h)]


def encode(p):
    out = []
    for row in p:
        lo = hi = 0
        for v in row:
            lo = (lo << 1) | (v & 1)
            hi = (hi << 1) | ((v >> 1) & 1)
        out.extend((lo, hi))
    return out


def grid(rows):
    return [[int(c) for c in row] for row in rows]


def put(p, x, y, c):
    if 0 <= y < len(p) and 0 <= x < len(p[0]):
        p[int(y)][int(x)] = c


def disc(p, cx, cy, r, c):
    for y in range(int(cy - r - 1), int(cy + r + 2)):
        for x in range(int(cx - r - 1), int(cx + r + 2)):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                put(p, x, y, c)


def ring(p, cx, cy, r, width, c, keep=lambda x, y: True):
    for y in range(int(cy - r - 1), int(cy + r + 2)):
        for x in range(int(cx - r - 1), int(cx + r + 2)):
            d = math.hypot(x - cx, y - cy)
            if r - width < d <= r and keep(x, y):
                put(p, x, y, c)


def rect(p, x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put(p, x, y, c)


def line(p, x0, y0, x1, y1, c, width=1):
    steps = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
    for i in range(steps + 1):
        t = i / steps
        x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
        for dy in range(width):
            for dx in range(width):
                put(p, round(x) + dx, round(y) + dy, c)


def poly(p, points, c):
    ys = [y for _, y in points]
    for y in range(int(min(ys)), int(max(ys)) + 1):
        xs = []
        for (x0, y0), (x1, y1) in zip(points, points[1:] + points[:1]):
            if (y0 <= y < y1) or (y1 <= y < y0):
                xs.append(x0 + (y - y0) * (x1 - x0) / (y1 - y0))
        xs.sort()
        for a, b in zip(xs[::2], xs[1::2]):
            for x in range(int(round(a)), int(round(b)) + 1):
                put(p, x, y, c)


def text(p, s, x, y, c, scale=1):
    for ch in s:
        for ry, row in enumerate(FONT.get(ch, ['00000'] * 7)):
            for rx, v in enumerate(row):
                if v == '1':
                    rect(p, x + rx * scale, y + ry * scale, x + rx * scale + scale - 1, y + ry * scale + scale - 1, c)
        x += 6 * scale


def card():
    """A 48 x 48 rounded card filled with colour 1."""
    p = canvas(48, 48)
    for y in range(48):
        for x in range(48):
            cx = min(max(x, 5.5), 41.5)
            cy = min(max(y, 5.5), 41.5)
            if (x - cx) ** 2 + (y - cy) ** 2 <= 36:
                p[y][x] = 1
    return p


def inside(p, x, y):
    return 0 <= x < 48 and 0 <= y < 48 and p[y][x] != 0


def icon_neon_wake():
    p = card()
    rect(p, 3, 22, 44, 22, 2)
    for y in range(8, 22, 3):
        for x in range(6 + y % 5, 42, 9):
            rect(p, x, y, x + 2, 21, 0)
    poly(p, [(22, 23), (25, 23), (46, 45), (1, 45)], 0)
    line(p, 22, 23, 4, 44, 2)
    line(p, 25, 23, 43, 44, 2)
    for y in (26, 32, 39):
        rect(p, 23, y, 24, y + 2 + (y - 26) // 5, 2)
    poly(p, [(17, 35), (30, 35), (33, 41), (14, 41)], 3)
    rect(p, 13, 41, 34, 43, 3)
    rect(p, 19, 36, 28, 38, 0)
    rect(p, 14, 42, 16, 43, 2)
    rect(p, 31, 42, 33, 43, 2)
    disc(p, 38, 12, 5, 3)
    disc(p, 40, 10, 4, 1)
    return p


def icon_moonthread():
    p = card()
    disc(p, 30, 17, 11, 3)
    disc(p, 34, 14, 9, 1)
    for x, y in [(8, 9), (14, 22), (40, 32), (9, 34), (22, 7)]:
        put(p, x, y, 2)
        put(p, x + 1, y, 3)
    line(p, 18, 3, 18, 29, 2)
    rect(p, 14, 30, 22, 32, 3)
    rect(p, 15, 33, 21, 37, 2)
    rect(p, 16, 34, 17, 35, 0)
    rect(p, 19, 34, 20, 35, 0)
    rect(p, 15, 38, 16, 40, 3)
    rect(p, 20, 38, 21, 40, 3)
    rect(p, 5, 42, 42, 44, 2)
    for x in range(6, 42, 6):
        rect(p, x, 42, x + 1, 44, 0)
    return p


def icon_echo_vault():
    p = card()
    for r in (8, 14, 20):
        ring(p, 16, 30, r, 1.6, 2, lambda x, y: x > 12 and y < 34)
    disc(p, 16, 30, 3, 3)
    for x0, y0, x1, y1 in [(4, 40, 43, 42), (30, 6, 32, 30), (36, 16, 43, 18), (22, 34, 24, 40)]:
        rect(p, x0, y0, x1, y1, 0)
    disc(p, 38, 29, 3.5, 3)
    disc(p, 38, 29, 1.5, 1)
    rect(p, 37, 32, 38, 38, 3)
    rect(p, 39, 35, 40, 36, 3)
    rect(p, 39, 37, 41, 38, 3)
    return p


def icon_bloom_circuit():
    p = card()
    for (x0, y0), (x1, y1) in [((24, 22), (24, 36)), ((24, 36), (12, 36)), ((12, 36), (12, 42)),
                               ((24, 32), (36, 32)), ((36, 32), (36, 42)), ((24, 36), (24, 42)),
                               ((12, 28), (18, 28)), ((18, 28), (18, 32))]:
        line(p, x0, y0, x1, y1, 2, 2)
    for x, y in [(12, 42), (36, 42), (24, 42), (12, 28)]:
        disc(p, x + 0.5, y + 0.5, 2.2, 3)
    for a in range(5):
        t = a * 2 * math.pi / 5 - math.pi / 2
        disc(p, 24.5 + 7 * math.cos(t), 14.5 + 7 * math.sin(t), 5, 3)
    disc(p, 24.5, 14.5, 3.5, 2)
    disc(p, 24.5, 14.5, 1.5, 0)
    poly(p, [(26, 24), (35, 20), (31, 27)], 2)
    return p


def icon_orbit_choir():
    p = card()
    for y in range(48):
        for x in range(48):
            e = ((x - 24) / 21) ** 2 + ((y - 26) / 7) ** 2
            if 0.82 < e <= 1.0 and inside(p, x, y):
                p[y][x] = 3
    disc(p, 24, 26, 11, 2)
    for y in range(15, 38):
        for x in range(13, 36):
            if (x - 24) ** 2 + (y - 26) ** 2 <= 121 and (x + y) % 6 == 0:
                p[y][x] = 1
    for y in range(48):
        for x in range(48):
            e = ((x - 24) / 21) ** 2 + ((y - 26) / 7) ** 2
            if 0.82 < e <= 1.0 and y > 26 and inside(p, x, y):
                p[y][x] = 3
    for nx, ny in [(8, 9), (36, 8)]:
        disc(p, nx, ny + 5, 2, 3)
        rect(p, nx + 1, ny - 1, nx + 2, ny + 4, 3)
        rect(p, nx + 3, ny - 1, nx + 4, ny, 3)
    return p


def icon_hello_dot():
    p = card()
    disc(p, 24, 26, 14, 3)
    rect(p, 17, 23, 19, 27, 0)
    rect(p, 28, 23, 30, 27, 0)
    rect(p, 18, 31, 29, 32, 0)
    rect(p, 17, 30, 18, 31, 0)
    rect(p, 29, 30, 30, 31, 0)
    for sx, sy in [(7, 9), (39, 8), (40, 38), (6, 38)]:
        rect(p, sx - 2, sy, sx + 2, sy, 2)
        rect(p, sx, sy - 2, sx, sy + 2, 2)
    return p


def icon_stormkite():
    p = card()
    for cx, cy, r in [(12, 12, 7), (22, 9, 8), (33, 11, 7), (40, 15, 5), (8, 16, 5)]:
        disc(p, cx, cy, r, 2)
    rect(p, 5, 15, 43, 18, 2)
    for x in range(6, 43):
        if inside(p, x, 19):
            p[19][x] = 1
    poly(p, [(19, 21), (17, 25), (21, 25), (18, 31), (25, 23), (21, 23), (23, 21)], 3)
    poly(p, [(33, 22), (42, 31), (33, 42), (24, 31)], 3)
    line(p, 33, 22, 33, 42, 1)
    line(p, 24, 31, 42, 31, 1)
    for i, (x, y) in enumerate([(31, 43), (28, 44), (24, 43), (20, 44)]):
        put(p, x, y, 3 if i % 2 else 2)
    return p


def icon_comet_links():
    p = card()
    disc(p, 13, 34, 9, 2)
    for y in range(24, 44):
        for x in range(3, 23):
            if (x - 13) ** 2 + (y - 34) ** 2 <= 81 and x + y > 49:
                p[y][x] = 1 if (x + y) % 2 else p[y][x]
    line(p, 9, 13, 30, 20, 2, 2)
    line(p, 13, 10, 31, 18, 2)
    disc(p, 33, 19, 3.5, 3)
    ring(p, 37, 37, 5, 1.5, 2)
    rect(p, 38, 24, 38, 36, 3)
    poly(p, [(39, 24), (45, 27), (39, 30)], 3)
    for x, y in [(6, 6), (24, 6), (42, 10), (26, 30), (41, 44)]:
        put(p, x, y, 3)
    return p


def icon_prism_well():
    p = card()
    rect(p, 8, 8, 10, 43, 2)
    rect(p, 37, 8, 39, 43, 2)
    rect(p, 8, 43, 39, 44, 2)
    gems = [(24, 13, 'diamond'), (24, 23, 'orb'), (24, 33, 'triangle')]
    for cx, cy, kind in gems:
        if kind == 'diamond':
            poly(p, [(cx, cy - 5), (cx + 5, cy), (cx, cy + 5), (cx - 5, cy)], 3)
            put(p, cx - 1, cy - 2, 2)
        elif kind == 'orb':
            disc(p, cx, cy, 4.5, 2)
            disc(p, cx - 1.5, cy - 1.5, 1.5, 3)
        else:
            poly(p, [(cx, cy - 5), (cx + 6, cy + 5), (cx - 6, cy + 5)], 3)
    for x in range(12, 36, 4):
        rect(p, x, 39, x + 2, 41, 1 if x % 8 else 0)
    rect(p, 12, 38, 35, 38, 0)
    for x, y in [(15, 16), (32, 20), (14, 28), (33, 34)]:
        put(p, x, y, 2)
    return p


def icon_dot_swarm():
    p=card()
    for cx,cy,r in [(9,34,3),(16,35,4),(24,31,4),(27,23,5),(34,17,8)]:
        disc(p,cx,cy,r,2)
        disc(p,cx-1,cy-2,1,3)
    put(p,32,15,0);put(p,36,15,0)
    for cx,cy in [(10,12),(16,20),(38,36)]:
        disc(p,cx,cy,2,3)
    return p

ICONS = [icon_neon_wake, icon_moonthread, icon_echo_vault, icon_bloom_circuit, icon_orbit_choir,
         icon_hello_dot, icon_stormkite, icon_comet_links, icon_prism_well, icon_dot_swarm]

UI = {
    'dot': ['00000000', '00000000', '00000000', '00011000', '00011000', '00000000', '00000000', '00000000'],
    'dot_on': ['00000000', '00000000', '00111100', '00133100', '00133100', '00111100', '00000000', '00000000'],
    'hairline': ['00000000', '00000000', '00000000', '00000000', '00000000', '00000000', '00000000', '11111111'],
    'star': ['00000000', '00000000', '00000000', '00000000', '00000100', '00000000', '00000000', '00000000'],
    'star2': ['00000000', '00010000', '00000000', '00000000', '00000000', '00000000', '00000000', '00000000'],
    'btn_a': ['00333300', '03311330', '33133133', '33111133', '33133133', '33133133', '03333330', '00333300'],
    'btn_b': ['00333300', '03111330', '33133133', '33111333', '33133133', '33111333', '03333330', '00333300'],
    'btn_sel': ['00000000', '00000000', '01111110', '13333331', '13233231', '13333331', '01111110', '00000000'],
    'bar': ['00000000', '00000000', '33333333', '33333333', '33333333', '33333333', '00000000', '00000000'],
    'bar_off': ['00000000', '00000000', '11111111', '10000000', '10000000', '11111111', '00000000', '00000000'],
}
# The brandmark: three skewed bars in teal, lilac and amber (16 x 16).
BRAND = grid([
    '0000000000000000',
    '0000111022203330',
    '0000111022203330',
    '0001110222033300',
    '0001110222033300',
    '0011102220333000',
    '0011102220333000',
    '0111022203330000',
    '0111022203330000',
    '1110222033300000',
    '1110222033300000',
    '0000000000000000',
    '0000000000000000',
    '0000000000000000',
    '0000000000000000',
    '0000000000000000',
])
SPRITES = {
    'corner': ['33330000', '31110000', '31000000', '31000000', '00000000', '00000000', '00000000', '00000000'],
    # The NEW badge spans two tiles; see badge() below.
    'new_l': None,
    'new_r': None,
    'arrow': ['00000000', '00010000', '00110000', '01110000', '00110000', '00010000', '00000000', '00000000'],
}


def c_bytes(kind, name, values):
    body = ''.join('    ' + ','.join(str(v) for v in values[i:i + 24]) + ',\n' for i in range(0, len(values), 24))
    return f'const {kind} {name}[] = {{\n{body}}};\n'


def rgb(c):
    r, g, b = c
    return r | (g << 5) | (b << 10)


def c_string(s):
    assert len(s) <= 20 and all(ch == ' ' or ch in FONT for ch in s), s
    return '"' + s + '"'


def controls(game):
    if 'controls' in game:
        return game['controls']
    source = (ROOT / 'games' / game['slug'] / 'src/main.c').read_text()
    return [re.search(name + r'\[\] = "([^"]*)"', source).group(1)
            for name in ('game_controls1', 'game_controls2', 'game_goal')]


def accent(game):
    import json
    info = {g['slug']: g for g in json.loads((ROOT / 'tools/games.json').read_text())}[game['slug']]
    h = info['accent'].lstrip('#')
    return tuple(int(h[i:i + 2], 16) >> 3 for i in (0, 2, 4))


def build():
    OUT.mkdir(exist_ok=True)
    bank0 = {}
    ui_tiles = []

    def ui(pixels):
        ui_tiles.append(encode(pixels))
        return 60 + len(ui_tiles) - 1
    ui_index = {key: ui(grid(rows)) for key, rows in UI.items()}
    brand = [ui([r[x:x + 8] for r in BRAND[y:y + 8]]) for y in (0, 8) for x in (0, 8)]
    assert 60 + len(ui_tiles) <= 80
    # Two-line logo for the boot card, like the game title screens.
    logo = canvas(160, 48)
    for i, word in enumerate(['CHROMATIC', 'FUN']):
        text(logo, word, (160 - len(word) * 12 + 2) // 2, 2 + i * 23, 3 if i == 0 else 2, 2)
    logo_tiles, logo_map = [], []
    for y in range(0, 48, 8):
        for x in range(0, 160, 8):
            t = encode([r[x:x + 8] for r in logo[y:y + 8]])
            if t not in logo_tiles:
                logo_tiles.append(t)
            logo_map.append(80 + logo_tiles.index(t))
    assert len(logo_tiles) <= 64, len(logo_tiles)
    # Icon tiles, deduplicated with flips, bank 1 first.
    known = {tuple(encode(canvas(8, 8))): (0, 0x08)}
    bank1_tiles, bank0_tiles = [encode(canvas(8, 8))], []
    icon_tiles, icon_attrs = [], []
    for draw in ICONS:
        p = draw()
        assert len(p) == 48 and len(p[0]) == 48
        for ty in range(6):
            for tx in range(6):
                b = [r[tx * 8:tx * 8 + 8] for r in p[ty * 8:ty * 8 + 8]]
                key = tuple(encode(b))
                if key not in known:
                    if len(bank1_tiles) < 256:
                        index, bank = len(bank1_tiles), 0x08
                        bank1_tiles.append(list(key))
                    else:
                        index, bank = 144 + len(bank0_tiles), 0
                        bank0_tiles.append(list(key))
                        assert index < 240, 'icon tiles exhausted'
                    known[key] = (index, bank)
                    for flip, variant in ((0x20, [r[::-1] for r in b]), (0x40, b[::-1]), (0x60, [r[::-1] for r in b[::-1]])):
                        known.setdefault(tuple(encode(variant)), (index, bank | flip))
                index, attr = known[key]
                icon_tiles.append(index)
                icon_attrs.append(attr)
    badge = grid([
        '0111111111111110',
        '1311310333131131',
        '1331310311131131',
        '1313310331131311',
        '1311310311133131',
        '1311310333131311',
        '0111111111111110',
        '0000000000000000',
    ])
    SPRITES['new_l'] = [''.join(str(v) for v in r[:8]) for r in badge]
    SPRITES['new_r'] = [''.join(str(v) for v in r[8:]) for r in badge]
    sprite_tiles = [encode(grid(rows)) for rows in SPRITES.values()]
    sprite_index = {key: 240 + i for i, key in enumerate(SPRITES)}

    art = '#pragma bank 21\n#include <gb/gb.h>\n#include <stdint.h>\n'
    art += c_bytes('uint8_t', 'menu_font', sum((font_tile(chr(c)) for c in range(32, 91)), []))
    art += c_bytes('uint8_t', 'menu_ui', sum(ui_tiles, []))
    art += c_bytes('uint8_t', 'menu_logo', sum(logo_tiles, []))
    art += c_bytes('uint8_t', 'menu_icons1', sum(bank1_tiles, []))
    art += c_bytes('uint8_t', 'menu_icons0', sum(bank0_tiles, []) or [0])
    art += c_bytes('uint8_t', 'menu_sprites', sum(sprite_tiles, []))
    art += f'''
void launcher_art(void) BANKED {{
  VBK_REG = 1;
  set_bkg_data(0, {len(bank1_tiles)}, menu_icons1);
  VBK_REG = 0;
  set_bkg_data(1, 59, menu_font);
  set_bkg_data(60, {len(ui_tiles)}, menu_ui);
  set_bkg_data(80, {len(logo_tiles)}, menu_logo);
  {'set_bkg_data(144, %d, menu_icons0);' % len(bank0_tiles) if bank0_tiles else ''}
  set_sprite_data(240, {len(sprite_tiles)}, menu_sprites);
}}
'''
    (OUT / 'launcher-art.c').write_text(art)

    data = '#include <stdint.h>\n#define GAME_COUNT %d\n' % len(GAMES)
    for key, value in ui_index.items():
        data += f'#define UI_{key.upper()} {value}\n'
    data += '#define UI_BRAND %d\n' % brand[0]
    for key, value in sprite_index.items():
        data += f'#define SPR_{key.upper()} {value}\n'
    data += c_bytes('uint8_t', 'menu_logo_map', logo_map)
    data += c_bytes('uint8_t', 'icon_tiles', icon_tiles)
    data += c_bytes('uint8_t', 'icon_attrs', icon_attrs)
    data += c_bytes('uint16_t', 'icon_palettes', [rgb(c) for g in GAMES for c in g['palette']])
    data += c_bytes('uint16_t', 'accents', [rgb(accent(g)) for g in GAMES])
    kinds = [255 if g['kind'] == 'hello' else g['kind'] for g in GAMES]
    data += c_bytes('uint8_t', 'game_kind', kinds)
    # fade_table[level * 32 + c] = c * level / 8, for palette fades.
    data += c_bytes('uint8_t', 'fade_table', [c * level >> 3 for level in range(9) for c in range(32)])
    for field in ('name', 'genre', 'tagline'):
        data += f'const char *const game_{field}s[] = {{' + ','.join(c_string(g[field]) for g in GAMES) + '};\n'
    data += 'const char *const game_blurbs[] = {' + ','.join(c_string(line) for g in GAMES for line in g['blurb']) + '};\n'
    data += 'const char *const game_help[] = {' + ','.join(c_string(line) for g in GAMES for line in controls(g)) + '};\n'
    (OUT / 'launcher-data.h').write_text(data)
    print(f'launcher art: {len(bank1_tiles)} bank-1 and {len(bank0_tiles)} bank-0 icon tiles, {len(logo_tiles)} logo tiles')
    return {'icons': [draw() for draw in ICONS]}


def font_tile(ch):
    p = canvas(8, 8)
    text(p, ch, 1, 0, 3)
    return encode(p)


if __name__ == '__main__':
    build()
    if len(sys.argv) > 1:
        # Optional preview sheet of every icon in its own palette.
        from PIL import Image
        sheet = Image.new('RGB', (len(GAMES) * 52, 52), tuple(v * 8 for v in INK))
        for i, (g, draw) in enumerate(zip(GAMES, ICONS)):
            p = draw()
            for y in range(48):
                for x in range(48):
                    sheet.putpixel((i * 52 + 2 + x, 2 + y), tuple(v * 8 for v in g['palette'][p[y][x]]))
        sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(sys.argv[1])
