"""Rival lab fleets. Every craft faces down the screen, towards the player.

Each lab has a drone (16x16), a gunner (16x16) and a heavy carrier (32x16),
two animation frames apiece. Colours: 1 rim/ink, 2 body, 3 light. The four
emblems are playful interpretations, not reproductions of real logos.
"""
import math
from gbgfx import Pic
from sprites import art


def shade_disc(p, cx, cy, r, rim=1, body=2, light=3, glint=True):
    for y in range(p.h):
        for x in range(p.w):
            dx, dy = x + 0.5 - cx, y + 0.5 - cy
            d = math.hypot(dx, dy)
            if d <= r:
                v = rim if d > r - 1.1 else body
                if glint and v == body and math.hypot(dx + r * 0.38, dy + r * 0.38) < r * 0.36:
                    v = light
                p.put(x, y, v)


def ray(p, cx, cy, angle, r0, r1, v, width=0.7):
    for y in range(p.h):
        for x in range(p.w):
            dx, dy = x + 0.5 - cx, y + 0.5 - cy
            along = dx * math.cos(angle) + dy * math.sin(angle)
            across = -dx * math.sin(angle) + dy * math.cos(angle)
            if r0 <= along <= r1 and abs(across) <= width:
                p.put(x, y, v)


def rim(p, v=1):
    """Outline opaque shapes in colour v (4-neighbour)."""
    return p.outline(v)


def star4(p, cx, cy, r, w, angle, v):
    """Four-point sparkle: a diamond with concave sides."""
    for y in range(p.h):
        for x in range(p.w):
            dx, dy = x + 0.5 - cx, y + 0.5 - cy
            a = dx * math.cos(angle) + dy * math.sin(angle)
            b = -dx * math.sin(angle) + dy * math.cos(angle)
            a, b = abs(a), abs(b)
            if (a <= r and b <= w * (1 - a / r) ** 1.6 * r / 2 + 0.5) or (b <= r and a <= w * (1 - b / r) ** 1.6 * r / 2 + 0.5):
                p.put(x, y, v)


def rival_jet(f):
    """Shared rival interceptor silhouette, nose down. Colour 2 body."""
    left = [
        '1.......',
        '21......',
        '221...11',
        '1221.122',
        '.1221222',
        '.1222222',
        '..122222',
        '..122222',
        '...12222',
        '...12222',
        '....1222',
        '....1222',
        '.....122',
        '.....122',
        '......12',
        '.......1',
    ]
    p = art(*[row + row[::-1] for row in left])
    if f:
        # Afterburner glints on the wingtips.
        p.put(0, 0, 3)
        p.put(15, 0, 3)
    return p


def emblem(p, kind, cx=8, cy=7.5, f=0):
    if kind == 0:   # orbital slash
        shade_disc(p, cx, cy, 3.6, glint=False)
        ray(p, cx, cy, math.radians(-45), -3.6, 3.6, 3, 0.6)
    elif kind == 1:  # sunburst
        for i in range(8):
            ray(p, cx, cy, math.radians(i * 45 + f * 22.5), 0, 3.6 if i % 2 == 0 else 2.6, 3, 0.5)
        p.put(int(cx) - 1, int(cy), 1)
    elif kind == 2:  # four-point star
        star4(p, cx, cy, 4.2, 1.3, 0, 3)
    else:            # M
        m = ['1...1', '11.11', '1.1.1', '1...1']
        for y, row in enumerate(m):
            for x, c in enumerate(row):
                p.put(int(cx) - 2 + x, int(cy) - 2 + y, 3 if c == '1' else 2)
    return p


# ------------------------------------------------------------------ Grok
def grok_drone(f):
    p = Pic(16, 16)
    shade_disc(p, 8, 8, 6.8)
    a = math.radians(-45 + f * 30)
    ray(p, 8, 8, a, -7, 7, 3, 0.9)
    ray(p, 8, 8, a, -4.8, 4.8, 1, 0.25)
    return p


def grok_gunner(f):
    return emblem(rival_jet(f), 0)


def grok_heavy(f):
    p = Pic(32, 16)
    # A ringed graphite sphere: the orbital slash emblem as a warship.
    for y in range(16):
        for x in range(32):
            dx, dy = (x + 0.5 - 16) / 15.5, (y + 0.5 - 9) / 4.2
            if dx * dx + dy * dy <= 1:
                p.put(x, y, 1 if dx * dx + dy * dy > 0.72 else 2)
    shade_disc(p, 16, 8, 7.4, glint=False)
    ray(p, 16, 8, math.radians(-40), -7.6, 7.6, 3, 0.9)
    for x in range(3, 30, 4):
        if p.get(x + f * 2, 9) == 2:
            p.put(x + f * 2, 9, 3)
    return p


def claude_drone(f):
    p = Pic(16, 16)
    for i in range(8):
        a = math.radians(i * 45)
        longer = (i % 2 == 0) != bool(f)
        ray(p, 8, 8, a, 2, 7.2 if longer else 5.4, 2, 0.95)
    shade_disc(p, 8, 8, 3.4, glint=False)
    p.put(7, 7, 3)
    rim(p)
    return p


def claude_gunner(f):
    return emblem(rival_jet(f), 1, f=f)


def claude_heavy(f):
    p = Pic(32, 16)
    # A radiant disc carrier: ten separate petals around a bright core.
    for i in range(10):
        a = math.radians(i * 36 + f * 18)
        px, py = 16 + math.cos(a) * 10.5, 8 + math.sin(a) * 4.6
        for y in range(16):
            for x in range(32):
                if ((x + 0.5 - px) / 3.0) ** 2 + ((y + 0.5 - py) / 2.1) ** 2 <= 1:
                    p.put(x, y, 2)
    shade_disc(p, 16, 8, 5.6)
    for i in range(8):
        ray(p, 16, 8, math.radians(i * 45 + 22.5), 1.2, 4.2, 3, 0.45)
    rim(p)
    return p


def gemini_drone(f):
    p = Pic(16, 16)
    r = 7.6 if f == 0 else 6.4
    star4(p, 8, 8, r, 1.6, 0, 2)
    star4(p, 8, 8, 4.2 if f == 0 else 5.0, 1.2, 0, 3)
    rim(p)
    return p


def gemini_gunner(f):
    return emblem(rival_jet(f), 2)


def gemini_heavy(f):
    p = Pic(32, 16)
    star4(p, 16, 8, 14.5, 1.4, 0, 2)
    star4(p, 16, 8, 7, 1.6, 0, 2)
    shade_disc(p, 16, 8, 4.4)
    star4(p, 16, 8, 3.4, 1.3, math.radians(45 * f), 3)
    for x in (6, 25):
        p.put(x + (1 if f else 0) * (1 if x < 16 else -1), 8, 3)
    rim(p)
    return p


# ------------------------------------------------------------------ Muse
def muse_drone(f):
    up = [
        '1..............1',
        '11............11',
        '121..........121',
        '1221........1221',
        '12221......12221',
        '122221....122221',
        '1223321..1233221',
        '12231321123132 1'.replace(' ', '2'),
        '1221.13223 1.1221'.replace(' ', '1')[:16],
        '121...1331...121',
        '11.....11.....11',
        '1..............1',
        '................',
        '................',
        '................',
        '................']
    down = [
        '................',
        '................',
        '1..............1',
        '11............11',
        '121..........121',
        '1221........1221',
        '12221......12221',
        '1223321..1233221',
        '122313211231322 1'.replace(' ', '')[:16],
        '.1221.1322 1.1221'.replace(' ', '3')[:16],
        '..121..1331..121',
        '...11...11...11.',
        '................',
        '................',
        '................',
        '................']
    return art(*(down if f else up))


def muse_gunner(f):
    return emblem(rival_jet(f), 3)


def muse_heavy(f):
    p = Pic(32, 16)
    # A bold M-shaped bomber with a lit bridge and engine pods.
    pts = [(1, 2), (9, 2), (16, 9), (23, 2), (31, 2), (31, 14), (24, 14), (24, 9), (16, 15), (8, 9), (8, 14), (1, 14)]
    p.poly(pts, 2)
    for x in range(3, 7):
        p.put(x, 4, 3)
        p.put(x + 22, 4, 3)
    for y in range(5, 12):
        p.put(4, y, 3 if y % 2 else 2)
        p.put(27, y, 3 if y % 2 else 2)
    for x in range(14, 18):
        p.put(x, 11, 3)
    p.put(15, 12, 1)
    p.put(16, 12, 1)
    for x in (2, 29):
        p.put(x, 13, 3 if f else 2)
        p.put(x + 1, 13, 3 if f else 2)
    rim(p)
    return p


FLEETS = [
    (grok_drone, grok_gunner, grok_heavy),
    (claude_drone, claude_gunner, claude_heavy),
    (gemini_drone, gemini_gunner, gemini_heavy),
    (muse_drone, muse_gunner, muse_heavy),
]


def fleet(lab):
    d, g, h = FLEETS[lab]
    return [d(0), d(1)], [g(0), g(1)], [h(0), h(1)]
