"""Scenery stamps and level layouts for the four sectors.

The world below the plane is a deep sky. Cloud banks, floating islands and
sky structures are painted as stamps at native resolution in named colours.
Each 8x8 tile picks one of six sector palettes. A level is a sorted list of
stamp placements on a 16-pixel grid; the cartridge composes map rows from it
while scrolling, so a long sector costs a few hundred bytes of layout.
"""
import math
import random
from gbgfx import Pic

# ------------------------------------------------------------- utilities


def noise2(x, y, seed=0):
    n = (x * 374761393 + y * 668265263 + seed * 2147483647) & 0xFFFFFFFF
    n = ((n ^ (n >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((n ^ (n >> 16)) & 0xFFFF) / 65535.0


def blob_mask(w, h, puffs):
    m = [[False] * w for _ in range(h)]
    for cx, cy, rx, ry in puffs:
        for y in range(h):
            for x in range(w):
                if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1:
                    m[y][x] = True
    return m


def inside(m, x, y):
    return 0 <= y < len(m) and 0 <= x < len(m[0]) and m[y][x]


# ---------------------------------------------------------------- clouds


def cloud(w, h, seed, colors):
    """A puffy cloud bank seen from above, lit from the upper left.

    colors: dict with 'shadow', 'mid', 'light' keys."""
    rnd = random.Random(seed)
    puffs = []
    # A core ellipse plus a ring of round puffs gives a cumulus outline.
    puffs.append((w / 2, h / 2, w / 2 - 6, h / 2 - 5))
    n = max(6, (w + h) // 7)
    for i in range(n):
        a = i / n * math.tau + rnd.uniform(-0.2, 0.2)
        r = rnd.uniform(4.5, 7.5)
        cx = w / 2 + math.cos(a) * (w / 2 - r - 1)
        cy = h / 2 + math.sin(a) * (h / 2 - r - 1)
        puffs.append((cx, cy, r, r * 0.9))
    for i in range(n // 2):
        puffs.append((rnd.uniform(w * 0.3, w * 0.7), rnd.uniform(h * 0.3, h * 0.7), rnd.uniform(5, 8), rnd.uniform(4, 7)))
    m = blob_mask(w, h, puffs)
    p = Pic(w, h)
    # Each puff's own upper-left highlight gives lumpy, sculpted volume.
    for y in range(h):
        for x in range(w):
            if not m[y][x]:
                continue
            v = colors['mid']
            if not inside(m, x, y + 1) or not inside(m, x + 1, y + 1) or not inside(m, x, y + 2):
                v = colors['shadow']
            else:
                for cx, cy, rx, ry in puffs[1:]:
                    dx, dy = (x + 0.5 - (cx - rx * 0.28)) / rx, (y + 0.5 - (cy - ry * 0.34)) / ry
                    if dx * dx + dy * dy < 0.26:
                        v = colors['light']
                        break
                if not inside(m, x, y - 1) or not inside(m, x - 1, y):
                    v = colors['light']
            p.px[y][x] = v
    # Soft dithered shadow band hugging the lower rim.
    for y in range(h):
        for x in range(w):
            if p.px[y][x] == colors['mid'] and not inside(m, x, y + 3) and (x + y) % 2 == 0:
                p.px[y][x] = colors['shadow']
    return p


def wisp(w, h, seed, colors):
    """A thin streak of high cloud."""
    rnd = random.Random(seed)
    puffs = [(w * t, h / 2 + rnd.uniform(-1, 1), rnd.uniform(3, 5), rnd.uniform(1.6, 2.6)) for t in (0.2, 0.35, 0.5, 0.65, 0.8)]
    m = blob_mask(w, h, puffs)
    p = Pic(w, h)
    for y in range(h):
        for x in range(w):
            if m[y][x]:
                p.px[y][x] = colors['light'] if not inside(m, x, y - 1) else (colors['shadow'] if not inside(m, x, y + 1) else colors['mid'])
    return p


# --------------------------------------------------------------- islands


def island(w, h, seed, c, deco='trees', top_h=None, cliff=10, falls=True):
    """A floating island seen from high above, with its south cliff showing.

    c: dict of named colours: dark, top, toplight, topdark?, rock, rocklight,
    water, waterlight. Trees use the top shades."""
    rnd = random.Random(seed)
    top_h = top_h or (h - cliff - 6)
    p = Pic(w, h)
    # Irregular top surface.
    puffs = [(w / 2, top_h / 2 + 1, w / 2 - 3, top_h / 2 - 2)]
    for i in range(7):
        a = i / 7 * math.tau + rnd.uniform(-0.3, 0.3)
        r = rnd.uniform(3.5, 6)
        puffs.append((w / 2 + math.cos(a) * (w / 2 - r - 2), top_h / 2 + 1 + math.sin(a) * (top_h / 2 - r - 1), r, r * 0.85))
    top = blob_mask(w, h, puffs)
    # Cliff: drop every top column downwards; length tapers to the edges.
    lowest = {}
    for x in range(w):
        ys = [y for y in range(h) if top[y][x]]
        if ys:
            lowest[x] = max(ys)
    xs = sorted(lowest)
    x0, x1 = xs[0], xs[-1]
    cliffmask = [[False] * w for _ in range(h)]
    for x in xs:
        t = (x - x0) / max(1, x1 - x0)
        depth = cliff * (1 - (2 * t - 1) ** 4) * (0.8 + 0.2 * noise2(x, 1, seed))
        depth = max(2, int(depth))
        # Rocky underside tapers into a jagged keel.
        keel = int((1 - abs(2 * t - 1)) ** 1.5 * 6 + noise2(x, 7, seed) * 2)
        for y in range(lowest[x] + 1, min(h, lowest[x] + 1 + depth + keel)):
            cliffmask[y][x] = True
    # Paint cliff with strata, lit on the left.
    for y in range(h):
        for x in range(w):
            if cliffmask[y][x] and not top[y][x]:
                v = c['rock']
                if not inside(cliffmask, x, y + 1):
                    v = c['dark']
                elif (y + int(noise2(x // 3, 0, seed) * 3)) % 4 == 0:
                    v = c['dark'] if noise2(x, y, seed) < 0.55 else c['rock']
                elif x < w * 0.45 and noise2(x, y, seed + 1) < 0.35:
                    v = c['rocklight']
                if not inside(cliffmask, x - 1, y) or not inside(cliffmask, x + 1, y):
                    v = c['dark']
                p.px[y][x] = v
    # Paint top: rim lip, grass texture.
    for y in range(h):
        for x in range(w):
            if top[y][x]:
                v = c['top']
                if not inside(top, x, y - 1) or not inside(top, x - 1, y):
                    v = c['toplight']
                elif not inside(top, x, y + 1):
                    v = c['dark'] if not inside(top, x, y + 2) and inside(cliffmask, x, y + 1) else c['top']
                elif not inside(top, x + 1, y):
                    v = c['dark']
                elif noise2(x, y, seed + 3) < (0.06 if deco == 'trees' else 0.03) and inside(top, x, y + 3):
                    v = c['toplight']
                elif noise2(x, y, seed + 4) < (0.04 if deco == 'trees' else 0.02) and inside(top, x, y + 3):
                    v = c['dark']
                p.px[y][x] = v
    # Ink outline where the island meets open sky.
    for y in range(h):
        for x in range(w):
            if p.px[y][x] is None and any(p.get(x + dx, y + dy) not in (None, c['dark']) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                if p.get(x, y - 1) is not None and p.get(x, y - 1) != c['dark'] and y > 0 and cliffmask[y - 1][x]:
                    p.px[y][x] = c['dark']
    if deco == 'trees':
        spots = []
        for _ in range(int(w * top_h / 110)):
            for _try in range(20):
                tx, ty = rnd.randint(4, w - 5), rnd.randint(4, top_h - 3)
                if all(top[yy][xx] for yy in range(ty - 3, ty + 3) for xx in range(tx - 3, tx + 4)) and \
                        all(abs(tx - a) + abs(ty - b) > 7 for a, b in spots):
                    spots.append((tx, ty))
                    break
        for tx, ty in sorted(spots, key=lambda s: s[1]):
            tree(p, tx, ty, c)
    if deco == 'rocks':
        rocks(p, top, rnd, c, max(2, w // 12))
    elif deco == 'crystals':
        crystals(p, top, rnd, c, max(2, w // 10))
        runes(p, top, rnd, c)
    if falls and c.get('water'):
        # A waterfall pours from the lip into the void.
        fx = int(x0 + (x1 - x0) * rnd.uniform(0.3, 0.7))
        top_y = lowest.get(fx, top_h) - 1
        for x in range(fx - 1, fx + 2):
            for y in range(top_y, h):
                if p.px[y][x] is None or cliffmask[y][x] or y <= lowest.get(x, 0):
                    edge = x != fx
                    p.px[y][x] = c['water'] if edge or y % 3 == 0 else c['waterlight']
        for x in range(fx - 2, fx + 3):
            if p.get(x, top_y - 1) is not None:
                p.put(x, top_y - 1, c['waterlight'])
    return p


def tree(p, x, y, c):
    shape = ['.###.', '#####', '#####', '.###.']
    for dy, row in enumerate(shape):
        for dx, ch in enumerate(row):
            if ch == '#':
                xx, yy = x - 2 + dx, y - 2 + dy
                v = c['top']
                if dx + dy <= 2:
                    v = c['toplight']
                if dx >= 3 and dy >= 2 or dy == 3:
                    v = c['dark']
                p.put(xx, yy, v)
    p.put(x + 2, y + 2, c['dark'])
    p.put(x + 1, y + 2, c['dark'])
    p.put(x - 1, y - 1, c['toplight'])


def text_stamp(s, pal_key='text'):
    """Font characters as a stamp: tile numbers are ASCII codes."""
    return [ord(ch) for ch in s]


def rocks(p, top, rnd, c, n):
    """Scattered boulders on a mesa top."""
    for _ in range(n):
        for _try in range(20):
            x, y = rnd.randint(4, p.w - 6), rnd.randint(4, len(top) - 4)
            if all(inside(top, xx, yy) for yy in range(y - 3, y + 3) for xx in range(x - 3, x + 4)):
                break
        else:
            continue
        for dy, row in enumerate(['.##.', '####', '####', '.##.']):
            for dx, ch in enumerate(row):
                if ch == '#':
                    v = c['toplight'] if dx + dy < 3 else (c['dark'] if dx + dy > 4 else c['rock'])
                    p.put(x - 2 + dx, y - 2 + dy, v)


def crystals(p, top, rnd, c, n):
    """Glowing shards on an obsidian platform."""
    for _ in range(n):
        for _try in range(20):
            x, y = rnd.randint(4, p.w - 5), rnd.randint(6, len(top) - 3)
            if all(inside(top, xx, yy) for yy in range(y - 6, y + 2) for xx in range(x - 2, x + 3)):
                break
        else:
            continue
        h = rnd.randint(4, 6)
        for dy in range(h):
            half = 1 if dy < h - 1 else 0
            for dx in range(-half, half + 1):
                p.put(x + dx, y - dy, c['crystal'] if dx >= 0 else c['crystallight'])
        p.put(x, y - h, c['crystallight'])
        p.put(x + 1, y + 1, c['dark'])


def runes(p, top, rnd, c):
    """Glowing lines etched across a platform."""
    w = p.w
    for y in range(2, len(top) - 2, 5):
        x = rnd.randint(4, 10)
        while x < w - 6:
            if inside(top, x, y) and inside(top, x + 3, y):
                for dx in range(3):
                    p.put(x + dx, y, c['glow'])
            x += rnd.randint(6, 12)


def city(w, h, seed, c):
    """A floating city block: deck, towers with lit windows, an engine keel."""
    rnd = random.Random(seed)
    p = Pic(w, h)
    deck_h = h - 14
    x0, x1 = 2, w - 3
    # Keel below the deck, tapering, with engine glows.
    for x in range(x0 + 2, x1 - 1):
        t = (x - x0) / (x1 - x0)
        depth = int(4 + (1 - abs(2 * t - 1)) * 8)
        for y in range(deck_h, min(h, deck_h + depth)):
            v = c['hull']
            if y == deck_h + depth - 1:
                v = c['dark']
            elif x in (x0 + 2, x1 - 2):
                v = c['dark']
            p.put(x, y, v)
    for gx in range(x0 + 6, x1 - 4, 8):
        t = (gx - x0) / (x1 - x0)
        gy = deck_h + int(4 + (1 - abs(2 * t - 1)) * 8) - 2
        p.put(gx, gy, c['glow'])
        p.put(gx + 1, gy, c['glow'])
    # Deck facade band with a row of windows.
    for x in range(x0, x1 + 1):
        for y in range(deck_h - 3, deck_h + 1):
            p.put(x, y, c['facade'] if y < deck_h else c['dark'])
        if x % 3 == 1:
            p.put(x, deck_h - 2, c['window'])
    # Deck surface.
    for y in range(1, deck_h - 3):
        for x in range(x0, x1 + 1):
            corner = (x - x0 < 2 or x1 - x < 2) and (y < 3)
            if corner:
                continue
            v = c['deck']
            if y == 1 or x == x0:
                v = c['decklight']
            elif x == x1:
                v = c['dark']
            p.put(x, y, v)
    # Towers, painted back to front: shallow roofs, tall lit facades.
    towers = []
    for _ in range(int(w * deck_h / 70) + 3):
        tw, td, th = rnd.randint(5, 9), rnd.randint(3, 5), rnd.randint(7, 15)
        tx = rnd.randint(x0 + 2, x1 - tw - 1)
        ty = rnd.randint(2, max(2, deck_h - 4 - td - th))
        if any(not (tx + tw + 1 < a or a + b + 1 < tx or ty + td + th < bb or bb + d + e < ty) for a, bb, b, d, e in towers):
            continue
        towers.append((tx, ty, tw, td, th))
    for tx, ty, tw, td, th in sorted(towers, key=lambda t: t[1] + t[3] + t[4]):
        for y in range(ty, ty + td):
            for x in range(tx, tx + tw):
                v = c['roof']
                if x == tx or y == ty:
                    v = c['decklight']
                if x == tx + tw - 1:
                    v = c['dark']
                p.put(x, y, v)
        neon = rnd.random() < 0.4
        for y in range(ty + td, ty + td + th):
            for x in range(tx, tx + tw):
                v = c['facade']
                if x == tx + tw - 1 or y == ty + td + th - 1:
                    v = c['dark']
                elif x == tx:
                    v = c['decklight'] if y == ty + td else c['facade']
                elif (x - tx) % 2 == 1 and (y - ty - td) % 3 == 1:
                    v = c['window'] if rnd.random() < 0.75 else c['dark']
                if neon and y == ty + td + th - 3 and tx < x < tx + tw - 1:
                    v = c['glow']
                p.put(x, y, v)
        if rnd.random() < 0.45:
            ax = tx + tw // 2
            for y in range(ty - 3, ty):
                p.put(ax, y, c['dark'])
            p.put(ax, ty - 4, c['glow'])
    return p
