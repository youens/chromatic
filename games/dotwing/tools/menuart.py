"""Menu art: Dot portraits, the hero plane, title clouds, icons, palettes."""
import math
from PIL import Image
from gbgfx import Pic, hexrgb
from sprites import art, PLANE
from palette import *  # noqa: F403

HUD_PANEL = '#101a48'
HUD_EDGE = '#3a54a8'

PANEL_FILL = '#14215a'
BACKDROP = '#0d1640'
# Menu background palettes. Text palettes put the panel fill at colour 0.
MENU_BG = [
    [BACKDROP, '#1b2c6e', '#2c4496', '#5a7ad0'],    # 0 backdrop and its stars
    [BACKDROP, '#050a24', '#5a82e0', PANEL_FILL],   # 1 panel frame
    [PANEL_FILL, '#050a24', '#a8c4f0', WHITE],      # 2 text
    [PANEL_FILL, '#050a24', '#ff9a2a', '#ffe070'],  # 3 gold text
    [PANEL_FILL, '#050a24', '#20b8c8', '#7cfaf0'],  # 4 selected text
    [PANEL_FILL, '#050a24', '#4a5c98', '#7a8cc4'],  # 5 dim text
    [PANEL_FILL, '#6e7894', '#ff7a5a', WHITE],      # 6 Grok and Claude emblems
    [PANEL_FILL, '#4a70f0', '#9a5cf0', WHITE],      # 7 Gemini and Muse emblems
]
MENU_OBJ = [
    [INK, INK, LIVERY[2], '#c8fae8'],          # 0 Dot body (livery set at runtime)
    [INK, INK, WHITE, '#ff8ab8'],              # 1 face
    [INK, INK, GOLD, '#bff4ff'],               # 2 gear
    [INK, INK, LIVERY[2], IVORY],              # 3 hero plane (livery set at runtime)
    [INK, INK, '#68c8ff', WHITE],              # 4 glass, cursor
    [INK, '#7aa8e8', '#d4ecff', WHITE],        # 5 clouds
    [INK, BROWN, GOLD, GOLDL],                 # 6 gold, sparkles
    [INK, '#c2281e', '#ff9a2a', GOLDL],        # 7 engine fire
]


def tint(c):
    r, g, b = hexrgb(c)
    return '#%02x%02x%02x' % tuple(min(255, int(v + (255 - v) * 0.62)) for v in (r, g, b))


# ------------------------------------------------------------ portraits
def body_mask(shape):
    def m(x, y):
        cx, cy = 16, 17.5
        dx, dy = x + 0.5 - cx, y + 0.5 - cy
        if shape == 0:      # ORB
            return dx * dx + dy * dy <= 13.6 ** 2
        if shape == 1:      # CUBE, rounded corners
            ax, ay = abs(dx), abs(dy)
            if ax > 12.5 or ay > 11.5:
                return False
            if ax > 8.5 and ay > 7.5:
                return (ax - 8.5) ** 2 + (ay - 7.5) ** 2 <= 16
            return True
        if shape == 2:      # DELTA, point up, rounded
            if dy < -12 or dy > 11:
                return False
            half = (dy + 12) * 0.62
            if abs(dx) > half:
                return False
            if dy > 7 and abs(dx) > half - 3:
                return (abs(dx) - (half - 3)) ** 2 + (dy - 7) ** 2 <= 16
            return True
        if shape == 3:      # PILL, vertical capsule
            ax = abs(dx)
            if ax > 10.5:
                return False
            ey = max(0.0, abs(dy) - 4)
            return ax * ax + ey * ey <= 10.5 ** 2
        # STAR: five rounded points
        a = math.atan2(dy, dx) + math.pi / 2
        r = math.hypot(dx, dy)
        k = (math.cos(5 * a) + 1) / 2
        return r <= 8.6 + 5.6 * k ** 1.6
    return m


def portrait_body(shape):
    m = body_mask(shape)
    p = Pic(32, 32)
    for y in range(32):
        for x in range(32):
            if not m(x, y):
                continue
            edge = not (m(x - 1, y) and m(x + 1, y) and m(x, y - 1) and m(x, y + 1))
            if edge:
                v = 1
            else:
                v = 2
                # Highlight crescent at the upper left.
                inner = m(x - 3, y - 3)
                if not inner and m(x + 2, y + 2):
                    v = 3
                # Shade dither at the lower right.
                if not m(x + 3, y + 3) and (x + y) % 2 == 0:
                    v = 1
            p.px[y][x] = v
    # Specular dot.
    for (x, y) in ((9, 9), (10, 9), (9, 10)):
        if p.get(x, y) == 2:
            p.put(x, y, 3)
    return p


FACE_ROWS = [
    # BRIGHT: big eyes, smile, blush
    ['................', '................', '................', '...11......11...', '..1121....1121..',
     '..1111....1111..', '..1111....1111..', '...11......11...', '.33..........33.', '.....1....1.....',
     '......1111......', '................', '................', '................', '................',
     '................'],
    # COOL: shades and a smirk
    ['................', '................', '................', '.11111111111111.', '.11211111112111.',
     '.11111111111111.', '..111111.11111..', '...1111...111...', '................', '................',
     '........1111....', '.......1........', '................', '................', '................',
     '................'],
    # WINK: one bright eye, a wink and an open grin
    ['................', '................', '................', '...1........11..', '..1.1......1121.',
     '.1...1.....1111.', '...........1111.', '............11..', '.33..........33.', '.....111111.....',
     '.....133331.....', '......1331......', '.......11.......', '................', '................',
     '................'],
    # FOCUS: brows, narrowed eyes, firm mouth
    ['................', '................', '..111......111..', '....11....11....', '...1111..1111...',
     '...1121..1211...', '....11....11....', '................', '................', '................',
     '......1111......', '................', '................', '................', '................',
     '................'],
]


def portrait_face(face):
    return Pic.ascii(FACE_ROWS[face], {'1': 1, '2': 2, '3': 3})


GEAR_ROWS = [
    ['.' * 32] * 16,
    # CAP: a rounded pilot cap with a brim and badge
    ['.' * 32, '.' * 32,
     '...........1111111111...........', '.........11222222222211.........',
     '........1222233222222221........', '.......122223333222222221.......',
     '.......122222332222222221.......', '......12222222222222222221......',
     '......12222222222222222221......', '.....1111111111111111111111.....',
     '....133333333333333333333331....', '.....1111111111111111111111.....',
     '.' * 32, '.' * 32, '.' * 32, '.' * 32],
    # HALO: a golden ring floating above
    ['.' * 32, '..........111111111111..........', '........11222222222222 11........'.replace(' ', '2'),
     '.......1233111111111112221.......', '.......1221...........1221.......', '........1122222222222211........',
     '..........111111111111..........', '.' * 32, '.' * 32, '.' * 32, '.' * 32, '.' * 32, '.' * 32,
     '.' * 32, '.' * 32, '.' * 32],
    # GOGGLES: two lenses on a strap across the brow
    ['.' * 32, '.' * 32, '.' * 32,
     '........1111......1111..........', '.......133331....133331.........',
     '..1111133333311113333331111111..', '..2222133223312213322331222222..',
     '..1111133333311113333331111111..', '.......133331....133331.........',
     '........1111......1111..........', '.' * 32, '.' * 32, '.' * 32, '.' * 32, '.' * 32, '.' * 32],
]
GEAR_Y = [0, 254, 250, 6]   # signed overlay offsets, two's complement


def portrait_gear(gear):
    rows = [r[:32].ljust(32, '.') for r in GEAR_ROWS[gear]]
    return Pic.ascii(rows, {'1': 1, '2': 2, '3': 3})


# ------------------------------------------------------------ hero plane
def scale2x(p):
    q = Pic(p.w * 2, p.h * 2)
    for y in range(p.h):
        for x in range(p.w):
            e = p.px[y][x]
            b, d, f, h = p.get(x, y - 1), p.get(x - 1, y), p.get(x + 1, y), p.get(x, y + 1)
            e0 = d if d == b and b != f and d != h else e
            e1 = f if b == f and b != d and f != h else e
            e2 = d if d == h and d != b and h != f else e
            e3 = f if h == f and d != h and b != f else e
            q.px[2 * y][2 * x], q.px[2 * y][2 * x + 1] = e0, e1
            q.px[2 * y + 1][2 * x], q.px[2 * y + 1][2 * x + 1] = e2, e3
    return q


HERO_LEFT = [
    '...............1',
    '..............13',
    '..............13',
    '.............133',
    '.............133',
    '.............133',
    '............1333',
    '............1332',
    '............1322',
    '...........13211',
    '...........12111',
    '...........11122',
    '...........11222',
    '...........11222',
    '...........11122',
    '...........13111',
    '...........13333',
    '..........133333',
    '.........1333332',
    '........13333332',
    '.......133333332',
    '......1333333332',
    '.....13223333333',
    '....132223333333',
    '...1322223333331',
    '..13222223333313',
    '.122222223333313',
    '1222222113333313',
    '.111111..1333333',
    '.........1332233',
    '........13322221',
    '.........1111.11',
]


def hero_plane():
    """32x32 hangar and title plane: ivory airframe, livery panels."""
    return Pic.ascii([r + r[::-1] for r in HERO_LEFT], {'1': 1, '2': 2, '3': 3})


def hero_glass():
    """Canopy shine and engine glow laid over the hero plane."""
    p = Pic(32, 32)
    for (x, y) in ((13, 10), (14, 10), (13, 11)):
        p.put(x, y, 3)
    p.put(18, 14, 2)
    return p


def title_cloud(kind):
    import terrain as T
    if kind == 0:
        c = T.cloud(32, 16, 41, {'shadow': 1, 'mid': 2, 'light': 3})
    else:
        c = Pic(16, 16)
        c.blit(T.cloud(16, 12, 42, {'shadow': 1, 'mid': 2, 'light': 3}), 0, 2)
    return c


def cursor():
    return Pic.ascii(['1.......', '31......', '331.....', '3331....', '33331...', '3331....', '331.....', '31......',
                      '1.......', '........', '........', '........', '........', '........', '........', '........'],
                     {'1': 1, '2': 2, '3': 3})


def swatch(index):
    """A colour-pick dot drawn in palette colour `index`."""
    rows = ['..xxxx..', '.xxxxxx.', 'xxxxxxxx', 'xxxxxxxx', 'xxxxxxxx', 'xxxxxxxx', '.xxxxxx.', '..xxxx..']
    rows += ['........'] * 8
    return Pic.ascii(rows, {'x': index})


def ring_cursor():
    rows = ['.1111111..', '1.......1.']
    return Pic.ascii(['..3333....'[:8], '.3....3.', '3......3', '3......3', '3......3', '3......3', '.3....3.', '..3333..',
                      '........', '........', '........', '........', '........', '........', '........', '........'],
                     {'3': 3})


def sparkle():
    return Pic.ascii(['...3....', '...3....', '..323...', '3322233.', '..323...', '...3....', '...3....', '........',
                      '........', '........', '........', '........', '........', '........', '........', '........'],
                     {'1': 1, '2': 2, '3': 3})


def boss_core(f):
    """The glowing eye each boss guards (hostile bullet palette)."""
    p = Pic(16, 16)
    for y in range(16):
        for x in range(16):
            d = math.hypot(x + 0.5 - 8, y + 0.5 - 8)
            if d <= 6.8:
                if d > 5.6:
                    v = 1
                elif d < (2.4 if f else 3.4):
                    v = 3
                else:
                    v = 2
                p.put(x, y, v)
    if f:
        for i in range(4):
            a = math.radians(45 + i * 90)
            p.put(int(8 + math.cos(a) * 4.4), int(8 + math.sin(a) * 4.4), 3)
    return p


# --------------------------------------------------------------- icons
def emblem_bg(kind):
    """16x16 lab emblem for background tiles: 1 dark, 2 colour, 3 white."""
    p = Pic(16, 16, 0)
    for y in range(16):
        for x in range(16):
            dx, dy = x + 0.5 - 8, y + 0.5 - 8
            d = math.hypot(dx, dy)
            if kind == 0:
                if 4.6 < d <= 7.2:
                    p.put(x, y, 1)
                if abs(dx * 0.7 + dy * 0.7) < 1.3 and d < 7.4:
                    p.put(x, y, 3)
            elif kind == 1:
                for i in range(8):
                    a = math.radians(i * 45)
                    along = dx * math.cos(a) + dy * math.sin(a)
                    across = -dx * math.sin(a) + dy * math.cos(a)
                    if 1.5 < along < 7.4 and abs(across) < 1.1:
                        p.put(x, y, 2)
                if d < 2.2:
                    p.put(x, y, 3)
            elif kind == 2:
                ax, ay = abs(dx), abs(dy)
                if (ax <= 7.4 and ay <= 2.2 * (1 - ax / 7.4) ** 1.4 * 3 + 0.4) or (ay <= 7.4 and ax <= 2.2 * (1 - ay / 7.4) ** 1.4 * 3 + 0.4):
                    p.put(x, y, 1 if (dx > 0) == (dy > 0) else 3)
            else:
                m = ['##......##', '###....###', '####..####', '##.####.##', '##..##..##', '##......##', '##......##']
                for yy, row in enumerate(m):
                    for xx, c in enumerate(row):
                        if c == '#':
                            p.put(3 + xx, 4 + yy, 2 if yy > 2 else 3)
    return p


def bg_icons():
    out = []
    for k in range(4):
        p = emblem_bg(k)
        cells = []
        for ty in (0, 8):
            for tx in (0, 8):
                cells.append([[p.px[ty + y][tx + x] or 0 for x in range(8)] for y in range(8)])
        out.append(('EMBLEM%d' % k, cells))
    return out


def previews(folder):
    sheet = Image.new('RGB', (5 * 40 + 8, 4 * 40 + 48), hexrgb(BACKDROP))
    for shape in range(5):
        for face in range(4):
            body = portrait_body(shape).image({1: INK, 2: LIVERY[shape], 3: tint(LIVERY[shape])})
            sheet.paste(body, (4 + shape * 40, 4 + face * 40), body)
            f = portrait_face(face).image({1: INK, 2: WHITE, 3: '#ff8ab8'})
            sheet.paste(f, (4 + shape * 40 + 8, 4 + face * 40 + 10), f)
            if face:
                g = portrait_gear(face).image({1: INK, 2: GOLD, 3: '#bff4ff'})
                oy = GEAR_Y[face] - (256 if GEAR_Y[face] > 127 else 0)
                sheet.paste(g, (4 + shape * 40, 4 + face * 40 + oy), g)
    hero = hero_plane().image({1: INK, 2: LIVERY[2], 3: IVORY})
    sheet.paste(hero, (4, 4 * 40 + 8), hero)
    glass = hero_glass().image({1: INK, 2: '#68c8ff', 3: WHITE})
    sheet.paste(glass, (4, 4 * 40 + 8), glass)
    sheet.resize((sheet.width * 3, sheet.height * 3), Image.Resampling.NEAREST).save(folder / 'dot-builder-sheet.png')


# --------------------------------------------------------------- title
SKY = ['#0a1446', '#111f62', '#1a2f80', '#24449e', '#3260bc', '#4a84d8', '#74aaee']
T_INK = '#050a24'
T_IVORY = '#fff2d6'
T_GOLD = '#ffc83a'
T_AMBER = '#e2801e'
T_CLOUD = ['#5a7ec8', '#a8c8f0', '#e4f2ff', WHITE]
T_ISLE = ['#203a7a', '#2c5a8a', '#3c7aa0']
TITLE_BG = [
    [SKY[0], SKY[1], '#8aa8ff', WHITE],          # 0 night sky, stars
    [SKY[1], SKY[2], SKY[3], SKY[0]],            # 1 upper gradient
    [SKY[3], SKY[4], SKY[5], SKY[6]],            # 2 lower gradient
    [SKY[0], T_INK, T_IVORY, WHITE],             # 3 logo top, white text
    [SKY[0], T_INK, T_AMBER, T_GOLD],            # 4 logo base, gold text
    [SKY[6], SKY[3], '#d8ecff', WHITE],          # 5 horizon text
    [SKY[6], T_CLOUD[0], T_CLOUD[1], T_CLOUD[2]],  # 6 clouds
    [SKY[3], T_ISLE[0], T_ISLE[1], T_ISLE[2]],     # 7 distant islands
]
TITLE_TEXT_ROW = 13      # flat horizon band for PRESS START
TITLE_INFO_ROW = 17      # dark strip for records


def logo():
    """DOTWING in doubled font strokes; the O is a smiling Dot."""
    from font import GLYPHS
    letters = 'DOTWING'
    widths = [len(GLYPHS[c][0]) * 2 for c in letters]
    w = sum(widths) + 2 * (len(letters) - 1) + 4
    p = Pic(w, 18)
    x = 1
    for c, cw in zip(letters, widths):
        if c == 'O':
            for yy in range(14):
                for xx in range(cw):
                    if (xx + 0.5 - cw / 2) ** 2 + (yy + 0.5 - 7) ** 2 <= 6.6 ** 2:
                        p.put(x + xx, 1 + yy, 'fill')
            for (ex, ey) in ((x + 3, 5), (x + 4, 5), (x + 3, 6), (x + 4, 6), (x + 8, 5), (x + 9, 5), (x + 8, 6), (x + 9, 6)):
                p.put(ex, ey, T_INK)
            for ex in range(x + 4, x + 9):
                p.put(ex, 10, T_INK)
            p.put(x + 3, 9, T_INK)
            p.put(x + 9, 9, T_INK)
        else:
            for yy, row in enumerate(GLYPHS[c]):
                for xx, ch in enumerate(row):
                    if ch == '#':
                        for dy in (0, 1):
                            for dx in (0, 1):
                                p.put(x + xx * 2 + dx, 1 + yy * 2 + dy, 'fill')
        x += cw + 2
    # Two-tone fill split at the tile boundary; ink outline and 3D shadow.
    out = Pic(w, 18)
    for yy in range(18):
        for xx in range(w):
            if p.px[yy][xx] == 'fill':
                if yy < 3:
                    v = WHITE
                elif yy < 7:
                    v = T_IVORY
                elif yy < 11:
                    v = T_GOLD
                else:
                    v = T_AMBER
                out.px[yy][xx] = v
            elif p.px[yy][xx] == T_INK:
                out.px[yy][xx] = T_INK
    for yy in range(18):
        for xx in range(w):
            if out.px[yy][xx] is None:
                near = any(p.get(xx + dx, yy + dy) is not None for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1), (-1, -1), (-2, -2), (-1, -2), (-2, -1)))
                if near:
                    out.px[yy][xx] = T_INK
    return out


def title_scene():
    import random
    rnd = random.Random(7)
    p = Pic(160, 144)
    # Night-to-day gradient with ordered dither between bands.
    bands = [(0, 48, 0), (48, 56, 1), (56, 64, 2), (64, 92, 3), (92, 98, 4), (98, 104, 5), (104, 144, 6)]
    bayer = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
    for y in range(144):
        for x in range(160):
            v = SKY[0]
            for y0, y1, k in bands:
                if y0 <= y < y1:
                    v = SKY[k]
                    if k and y - y0 < 4 and bayer[y & 3][x & 3] >= (y - y0) * 4:
                        v = SKY[k - 1]
            p.px[y][x] = v
    # Stars in the night band, clear of the logo and tagline.
    for _ in range(46):
        x, y = rnd.randrange(160), rnd.randrange(46)
        if 10 <= y <= 47 and 14 <= x <= 146:
            continue
        p.put(x, y, WHITE if rnd.random() < 0.4 else '#8aa8ff')
    for (x, y) in ((8, 6), (150, 12), (138, 3), (20, 40)):
        for d in (-1, 1):
            p.put(x + d, y, '#8aa8ff')
            p.put(x, y + d, '#8aa8ff')
        p.put(x, y, WHITE)
    # Distant floating islands, hazy with altitude.
    for cx, cy, w in ((24, 78, 30), (136, 76, 24), (98, 82, 14)):
        for y in range(cy - 5, cy + 12):
            for x in range(cx - w // 2 - 2, cx + w // 2 + 3):
                dx = (x + 0.5 - cx) / (w / 2)
                top = cy - 4 + 3 * dx * dx
                keel = cy + 1 + (1 - abs(dx)) * 8
                if abs(dx) <= 1 and top <= y <= keel:
                    v = T_ISLE[1]
                    if y < top + 2:
                        v = T_ISLE[2]
                    elif y > cy + 1:
                        v = T_ISLE[0]
                    p.put(x, y, v)
    # The cloud sea: layered cumulus puffs, painted back to front.
    puffs = []
    for row, (y0, r0) in enumerate(((115, 7), (121, 9), (128, 10), (135, 11))):
        x = -6 + rnd.randint(0, 8)
        while x < 170:
            r = r0 + rnd.uniform(-2, 2)
            puffs.append((x, y0 + rnd.uniform(-2, 2), r))
            x += r * 1.5 + rnd.uniform(0, 4)
    for cx, cy, r in puffs:
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                if not (0 <= x < 160 and 112 <= y < TITLE_INFO_ROW * 8):
                    continue
                dx, dy = x + 0.5 - cx, y + 0.5 - cy
                d = (dx * dx + dy * dy) ** 0.5
                if d > r:
                    continue
                if d > r - 1.5 and dy > -r * 0.2:
                    v = T_CLOUD[0]
                elif (dx + r * 0.3) ** 2 + (dy + r * 0.45) ** 2 < (r * 0.55) ** 2:
                    v = T_CLOUD[2]
                else:
                    v = T_CLOUD[1]
                p.px[y][x] = v
    for y in range(112, TITLE_INFO_ROW * 8):
        for x in range(160):
            if p.px[y][x] == SKY[6] and y > 120:
                p.px[y][x] = T_CLOUD[0]
    # Flat bands for text.
    for y in range(TITLE_TEXT_ROW * 8, TITLE_TEXT_ROW * 8 + 8):
        for x in range(160):
            p.px[y][x] = SKY[6]
    for y in range(TITLE_INFO_ROW * 8, 144):
        for x in range(160):
            p.px[y][x] = SKY[0]
    lg = logo()
    p.blit(lg, (160 - lg.w) // 2, 13)
    return p
