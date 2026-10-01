"""Background-layer bosses, 96x56 pixels, lit from the upper left.

A boss is drawn into the background map over a flat arena; scrolling the
background moves it. Five palettes (1-5) colour it; palette 0 is the arena.
Each boss also defines its core: a small tile block that is redrawn when the
core opens, and the hitbox the cartridge uses for player shots.
"""
import math
from gbgfx import Pic

W, H = 96, 56


def shade_sphere(p, cx, cy, r, cols, mask=None, ry=None):
    """cols: (rim, dark, mid, light, glint)."""
    ry = ry or r
    rim, dark, mid, light, glint = cols
    for y in range(p.h):
        for x in range(p.w):
            dx, dy = (x + 0.5 - cx) / r, (y + 0.5 - cy) / ry
            d = dx * dx + dy * dy
            if d > 1 or (mask and not mask(x, y)):
                continue
            # Lambert-ish term with light from the upper left.
            nz = math.sqrt(max(0.0, 1 - d))
            lum = (-dx * 0.55 - dy * 0.65 + nz * 0.55)
            if d > 0.86:
                v = rim
            elif lum > 0.78:
                v = glint
            elif lum > 0.42:
                v = light
            elif lum > 0.05:
                v = mid
            else:
                v = dark
            p.put(x, y, v)


def ring(p, cx, cy, rx_out, ry_out, rx_in, ry_in, cols, front):
    rim, mid, light = cols
    for y in range(p.h):
        for x in range(p.w):
            dx, dy = x + 0.5 - cx, y + 0.5 - cy
            o = (dx / rx_out) ** 2 + (dy / ry_out) ** 2
            i = (dx / rx_in) ** 2 + (dy / ry_in) ** 2
            if o <= 1 and i > 1 and ((dy >= 0) == front):
                v = mid
                if o > 0.86 or i < 1.18:
                    v = rim
                elif dy < -ry_in * 0.2 or (dy > 0 and dy < ry_out * 0.45 and o < 0.7):
                    v = light
                p.put(x, y, v)


def grok():
    base = '#2a58c4'
    ink, gd, gm, gl, wh = '#0a1030', '#363c58', '#6a7494', '#aab8d8', '#ffffff'
    silver, sdark = '#d0d8f0', '#7a86a8'
    red, glow = '#ff2a6a', '#ffb8d0'
    p = Pic(W, H)
    ring(p, 48, 30, 47, 13, 37, 7, (ink, sdark, silver), front=False)
    shade_sphere(p, 48, 28, 26, (ink, gd, gm, gl, wh))
    # The slash: a broad diagonal blade across the sphere.
    for y in range(H):
        for x in range(W):
            dx, dy = x + 0.5 - 48, y + 0.5 - 28
            if dx * dx + dy * dy < 24.5 ** 2:
                a = (dx * 0.62 + dy * 0.78)
                if abs(a) < 2.6:
                    p.put(x, y, wh if a < 1.2 else silver)
                elif abs(a) < 3.6:
                    p.put(x, y, ink)
    ring(p, 48, 30, 47, 13, 37, 7, (ink, sdark, silver), front=True)
    # Gun ports along the ring.
    for gx in (10, 22, 74, 86):
        gy = 30 + int(13 * 0.62 * math.sqrt(max(0, 1 - ((gx - 48) / 47) ** 2)))
        for yy in range(gy - 1, gy + 2):
            for xx in range(gx - 1, gx + 2):
                p.put(xx, yy, ink)
        p.put(gx, gy, red)
    palettes = [
        [base, ink, gm, gl],
        [base, ink, sdark, silver],
        [gm, ink, gl, wh],
        [gd, ink, gm, gl],
        [gd, ink, red, glow],
    ]
    return p, palettes, (40, 20), base


def core_eye(p, cx, cy, ink, red, glow, open_):
    for y in range(cy - 6, cy + 6):
        for x in range(cx - 6, cx + 6):
            dx, dy = x + 0.5 - cx, y + 0.5 - cy
            d = math.hypot(dx, dy)
            if d < 5.6:
                if not open_:
                    p.put(x, y, ink if d > 4.4 or abs(dy) > 1.2 else red)
                else:
                    p.put(x, y, ink if d > 4.6 else (glow if d < 2.2 else red))


def claude():
    base = '#5a3a8a'
    ink, rd, coral, peach, cream = '#1a0a2a', '#8a2a2a', '#ff7a5a', '#ffc4a0', '#fff4e4'
    gold, amber = '#ffd060', '#e8781e'
    p = Pic(W, H)
    # Twelve tapering petals, stretched horizontally.
    for i in range(12):
        a = math.radians(i * 30 + 15)
        for y in range(H):
            for x in range(W):
                dx, dy = (x + 0.5 - 48) / 1.75, y + 0.5 - 28
                along = dx * math.cos(a) + dy * math.sin(a)
                across = -dx * math.sin(a) + dy * math.cos(a)
                if 14 <= along <= 27.5 and abs(across) <= 4.6 * (1 - (along - 14) / 15):
                    v = coral
                    if across < -1.2:
                        v = peach
                    if along > 25.5 or abs(across) > 3.9 * (1 - (along - 14) / 15):
                        v = rd
                    p.put(x, y, v)
    shade_sphere(p, 48, 28, 19, (ink, rd, coral, peach, cream))
    # Inner sunburst.
    for i in range(8):
        a = math.radians(i * 45)
        for y in range(H):
            for x in range(W):
                dx, dy = x + 0.5 - 48, y + 0.5 - 28
                along = dx * math.cos(a) + dy * math.sin(a)
                across = -dx * math.sin(a) + dy * math.cos(a)
                if 6 <= along <= 14 and abs(across) <= 1.6 * (1 - (along - 6) / 9):
                    p.put(x, y, cream if along < 10 else gold)
    for y in range(H):
        for x in range(W):
            if p.get(x, y) is None and any(p.get(x + dx, y + dy) not in (None, ink) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                p.put(x, y, ink)
    palettes = [
        [base, ink, rd, coral],
        [base, ink, coral, peach],
        [coral, rd, peach, cream],
        [coral, ink, gold, cream],
        [rd, ink, amber, gold],
    ]
    return p, palettes, (40, 20), base


def star4_mask(cx, cy, r, w):
    def m(x, y):
        dx, dy = abs(x + 0.5 - cx), abs(y + 0.5 - cy)
        return (dx <= r and dy <= w * (1 - dx / r) ** 1.5 * r / 2 + 0.6) or (dy <= r and dx <= w * (1 - dy / r) ** 1.5 * r / 2 + 0.6)
    return m


def gemini():
    base = '#14204a'
    ink, ind, blue, sky, ice, wh = '#060a20', '#283c9a', '#4a70f0', '#8ab8ff', '#d0f0ff', '#ffffff'
    violet, lilac = '#7a4cf0', '#d8c0ff'
    p = Pic(W, H)
    # A bridge joins the twin crystals.
    for y in range(22, 34):
        for x in range(20, 76):
            v = ind if y > 30 else (blue if y > 24 else sky)
            if y in (22, 33):
                v = ink
            p.put(x, y, v)
    for cx in (24, 72):
        m = star4_mask(cx, 28, 25, 1.25)
        for y in range(H):
            for x in range(W):
                if m(x, y):
                    dx, dy = x + 0.5 - cx, y + 0.5 - 28
                    # Facets: four quadrants catch light differently.
                    if dx < 0 and dy < 0:
                        v = ice
                    elif dx >= 0 and dy < 0:
                        v = sky
                    elif dx < 0:
                        v = blue
                    else:
                        v = ind
                    if abs(dx) < 1 or abs(dy) < 1:
                        v = wh if dy < 0 or dx < 0 else sky
                    p.put(x, y, v)
        for y in range(H):
            for x in range(W):
                if p.get(x, y) is None and m(x - 1, y) or p.get(x, y) is None and m(x + 1, y) or p.get(x, y) is None and m(x, y - 1) or p.get(x, y) is None and m(x, y + 1):
                    p.put(x, y, ink)
    shade_sphere(p, 48, 28, 7.5, (ink, violet, violet, lilac, wh))
    palettes = [
        [base, ink, ind, blue],
        [base, ink, sky, ice],
        [ind, ink, blue, sky],
        [sky, ice, wh, blue],
        [ind, ink, violet, lilac],
    ]
    return p, palettes, (44, 24), base


def muse():
    base = '#1e0f3e'
    ink, dv, purple, lav, wh = '#0a0418', '#3a1a78', '#8a4cf0', '#c8a8ff', '#ffffff'
    mag, pinkl = '#ff4ac8', '#ffc0ec'
    p = Pic(W, H)
    pts = [(2, 6), (24, 6), (48, 30), (72, 6), (94, 6), (94, 50), (70, 50), (70, 30), (48, 52), (26, 30), (26, 50), (2, 50)]
    p.poly(pts, purple)
    # Bevel: light top-left faces, dark bottom-right faces.
    src = p.copy()
    for y in range(H):
        for x in range(W):
            if src.px[y][x] is None:
                continue
            if src.get(x, y - 1) is None or src.get(x - 1, y) is None or src.get(x, y - 2) is None:
                p.px[y][x] = lav
            elif src.get(x, y + 1) is None or src.get(x + 1, y) is None or src.get(x, y + 2) is None:
                p.px[y][x] = dv
    # Panel lines and window rows.
    for x in range(6, 22):
        if x % 3:
            p.put(x, 12, mag)
            p.put(x + 68, 12, mag)
    for y in range(16, 46, 6):
        for x in (8, 12, 16, 20, 76, 80, 84, 88):
            p.put(x, y, pinkl)
            p.put(x + 1, y, mag)
    for y in range(H):
        for x in range(W):
            if p.get(x, y) is None and any(src.get(x + dx, y + dy) is not None for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                p.put(x, y, ink)
    palettes = [
        [base, ink, dv, purple],
        [base, ink, purple, lav],
        [purple, dv, lav, wh],
        [purple, dv, mag, pinkl],
        [dv, ink, mag, pinkl],
    ]
    return p, palettes, (40, 28), base


BOSSES = [grok, claude, gemini, muse]
