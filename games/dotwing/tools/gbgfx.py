"""Small exact-pixel toolkit for Game Boy Color art.

Art is authored at native resolution. A Pic is a grid of colour keys (None is
transparent). Sprite art maps keys onto one 4-colour palette; background art
may span several palettes, and each 8x8 tile picks the first palette that
holds every colour it uses. Tiles are deduplicated, including mirror images,
because CGB map attributes can flip a background tile.
"""
from PIL import Image


def hexrgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


def cgb(h):
    r, g, b = hexrgb(h)
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


class Pic:
    def __init__(self, w, h, fill=None):
        self.w, self.h = w, h
        self.px = [[fill] * w for _ in range(h)]

    @classmethod
    def ascii(cls, rows, key):
        rows = [r for r in rows]
        width = max(len(r) for r in rows)
        p = cls(width, len(rows))
        for y, row in enumerate(rows):
            for x, c in enumerate(row):
                p.px[y][x] = key.get(c)
        return p

    def copy(self):
        p = Pic(self.w, self.h)
        p.px = [r[:] for r in self.px]
        return p

    def get(self, x, y, default=None):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.px[y][x]
        return default

    def put(self, x, y, v):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = v

    def rect(self, x, y, w, h, v):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.put(xx, yy, v)

    def ellipse(self, cx, cy, rx, ry, v, test=None):
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            for x in range(int(cx - rx - 1), int(cx + rx + 2)):
                dx = (x + 0.5 - cx) / rx
                dy = (y + 0.5 - cy) / ry
                if dx * dx + dy * dy <= 1.0 and (test is None or test(x, y)):
                    self.put(x, y, v)

    def poly(self, pts, v):
        """Even-odd fill of a polygon sampled at pixel centres."""
        ys = [p[1] for p in pts]
        for y in range(int(min(ys)) - 1, int(max(ys)) + 2):
            cy = y + 0.5
            xs = []
            for i in range(len(pts)):
                (x1, y1), (x2, y2) = pts[i], pts[(i + 1) % len(pts)]
                if (y1 <= cy < y2) or (y2 <= cy < y1):
                    xs.append(x1 + (cy - y1) * (x2 - x1) / (y2 - y1))
            xs.sort()
            for a, b in zip(xs[::2], xs[1::2]):
                for x in range(int(round(a)), int(round(b))):
                    self.put(x, y, v)

    def line(self, x1, y1, x2, y2, v):
        n = max(abs(x2 - x1), abs(y2 - y1), 1)
        for i in range(n + 1):
            self.put(round(x1 + (x2 - x1) * i / n), round(y1 + (y2 - y1) * i / n), v)

    def blit(self, other, ox, oy, transparent=True):
        for y in range(other.h):
            for x in range(other.w):
                v = other.px[y][x]
                if v is not None or not transparent:
                    self.put(ox + x, oy + y, v)

    def mirror_left(self):
        """Copy the left half onto the right half, mirrored."""
        for y in range(self.h):
            for x in range(self.w // 2):
                self.px[y][self.w - 1 - x] = self.px[y][x]
        return self

    def flip_h(self):
        p = self.copy()
        p.px = [r[::-1] for r in p.px]
        return p

    def flip_v(self):
        p = self.copy()
        p.px = p.px[::-1]
        return p

    def crop(self, x, y, w, h):
        p = Pic(w, h)
        for yy in range(h):
            for xx in range(w):
                p.px[yy][xx] = self.get(x + xx, y + yy)
        return p

    def outline(self, v, inside=None, diagonal=False):
        """Ring every opaque region with colour v on transparent neighbours."""
        src = self.copy()
        for y in range(self.h):
            for x in range(self.w):
                if src.px[y][x] is not None:
                    continue
                n = [(1, 0), (-1, 0), (0, 1), (0, -1)]
                if diagonal:
                    n += [(1, 1), (-1, 1), (1, -1), (-1, -1)]
                if any(src.get(x + dx, y + dy) not in (None, v) and (inside is None or src.get(x + dx, y + dy) in inside) for dx, dy in n):
                    self.px[y][x] = v
        return self

    def replace(self, a, b):
        for row in self.px:
            for i, v in enumerate(row):
                if v == a:
                    row[i] = b
        return self

    def keys(self):
        return {v for row in self.px for v in row if v is not None}

    def image(self, colors, scale=1, bg=None):
        im = Image.new('RGBA', (self.w, self.h), (0, 0, 0, 0) if bg is None else hexrgb(bg) + (255,))
        for y, row in enumerate(self.px):
            for x, v in enumerate(row):
                if v is not None:
                    c = colors[v] if not isinstance(v, tuple) else v
                    im.putpixel((x, y), (hexrgb(c) if isinstance(c, str) else c) + (255,))
        if scale != 1:
            im = im.resize((self.w * scale, self.h * scale), Image.Resampling.NEAREST)
        return im


def distance(a, b):
    """Perceptual-ish distance between two #rrggbb colours."""
    if a == b:
        return 0
    (r1, g1, b1), (r2, g2, b2) = hexrgb(a), hexrgb(b)
    rm = (r1 + r2) / 2
    return ((2 + rm / 256) * (r1 - r2) ** 2 + 4 * (g1 - g2) ** 2 + (2 + (255 - rm) / 256) * (b1 - b2) ** 2) ** 0.5


def encode_tile(rows):
    """rows: 8 lists of 8 colour indices 0..3 -> 16 bytes of 2bpp."""
    out = []
    for row in rows:
        lo = hi = 0
        for v in row:
            lo = (lo << 1) | (v & 1)
            hi = (hi << 1) | ((v >> 1) & 1)
        out += [lo, hi]
    return out


def sprite_tiles(pic, palette_keys):
    """Encode a sprite Pic (width multiple of 8, height multiple of 16) in
    8x16 column-major order. palette_keys lists keys for indices 1..3."""
    index = {None: 0}
    for i, k in enumerate(palette_keys):
        if k is not None:
            index[k] = i + 1
    assert pic.w % 8 == 0 and pic.h % 16 == 0, (pic.w, pic.h)
    data = []
    for ty in range(0, pic.h, 16):
        for tx in range(0, pic.w, 8):
            for sub in (0, 8):
                rows = []
                for y in range(8):
                    row = []
                    for x in range(8):
                        v = pic.px[ty + sub + y][tx + x]
                        assert v in index, ('colour not in sprite palette', v, palette_keys)
                        row.append(index[v])
                    rows.append(row)
                data += encode_tile(rows)
    return data


class BgSheet:
    """Deduplicating background tile store with per-tile palette selection."""

    def __init__(self, palettes, base=0, limit=256, flips=True):
        # palettes: list of 4-key lists. Index 0 is the shared backdrop key.
        self.palettes = palettes
        self.tiles = []        # tuples of 64 indices
        self.lookup = {}
        self.base = base
        self.limit = limit
        self.flips = flips
        self.quantised = []

    def tile_for(self, pic, x0, y0, prefer=None, fill=None):
        """Return (tile_number, flip, palette) for the 8x8 block at x0, y0.

        Transparent pixels become `fill`. If no palette holds every colour,
        the tile is quantised to the palette with the least colour error and
        the substitution is recorded in self.quantised."""
        block = [[pic.get(x0 + x, y0 + y) for x in range(8)] for y in range(8)]
        block = [[fill if v is None else v for v in row] for row in block]
        used = {v for row in block for v in row}
        order = list(range(len(self.palettes)))
        if prefer is not None:
            order.remove(prefer)
            order.insert(0, prefer)
        for p in order:
            pal = self.palettes[p]
            if used <= set(pal):
                idx = [[pal.index(v) for v in row] for row in block]
                return self.add(idx) + (p,)
        best = None
        for p in order:
            pal = self.palettes[p]
            cost = sum(min(distance(v, c) for c in pal) for row in block for v in row)
            if best is None or cost < best[0]:
                best = (cost, p)
        pal = self.palettes[best[1]]
        idx = [[min(range(4), key=lambda i: distance(v, pal[i])) for v in row] for row in block]
        self.quantised.append((x0, y0, best[0]))
        return self.add(idx) + (best[1],)

    def add(self, idx):
        key = tuple(v for row in idx for v in row)
        variants = [(key, 0)]
        if self.flips:
            h = tuple(v for row in idx for v in row[::-1])
            v = tuple(v for row in idx[::-1] for v in row)
            hv = tuple(v for row in idx[::-1] for v in row[::-1])
            variants += [(h, 0x20), (v, 0x40), (hv, 0x60)]
        for k, flip in variants:
            if k in self.lookup:
                return self.lookup[k], flip
        n = self.base + len(self.tiles)
        assert len(self.tiles) < self.limit, 'tile budget exceeded'
        self.tiles.append(key)
        self.lookup[key] = n
        return n, 0

    def data(self):
        out = []
        for t in self.tiles:
            out += encode_tile([list(t[y * 8:y * 8 + 8]) for y in range(8)])
        return out

    def map(self, pic, prefer=None, fill=None):
        """Tile map for a whole Pic: rows of (sheet_index, flip, palette)."""
        rows = []
        for ty in range(0, pic.h, 8):
            row = []
            for tx in range(0, pic.w, 8):
                row.append(self.tile_for(pic, tx, ty, prefer, fill))
            rows.append(row)
        return rows


def c_array(name, data, ctype='uint8_t', per_line=24):
    body = ''.join('  ' + ','.join(str(v) for v in data[i:i + per_line]) + ',\n'
                   for i in range(0, len(data), per_line))
    return f'static const {ctype} {name}[] = {{\n{body}}};\n'


def c_palettes(name, colors):
    return f'static const palette_color_t {name}[] = {{' + ','.join(hex(cgb(c)) for c in colors) + '};\n'
