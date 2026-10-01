"""Four sectors: palettes, scenery stamps and generated level layouts.

Level rows count upwards from the bottom of the opening screen. A placement
(row, col, stamp, hflip) anchors a stamp's bottom-left tile; the cartridge
composes each new map row from the base pattern and any active stamps.
The level is 24 tiles wide so the camera can slide 16 pixels either way.
"""
import random
from PIL import Image
from gbgfx import BgSheet, hexrgb
import terrain as T

WIDTH = 24          # level columns
OPEN_ROWS = 18      # opening screen
ROWS = 300          # rows of scenery before the boss arena
ARENA = 40          # flat rows that follow; the boss fights over them


def base_hash(col, row):
    """Must match dw_base_tile() in src/world.c."""
    return ((row * 7) ^ (col * 13) ^ (row >> 3)) & 31


def base_tiles(speck):
    """Eight 8x8 base tiles: flat, then sparse far-below details."""
    flat = [[0] * 8 for _ in range(8)]
    tiles = [flat]
    marks = [
        ['........', '........', '...1....', '..121...', '...1....', '........', '........', '........'],
        ['........', '........', '........', '.11..11.', '..1111..', '........', '........', '........'],
        ['........', '........', '..1111..', '.122221.', '..1111..', '........', '........', '........'],
        ['........', '....2...', '........', '........', '........', '.1......', '........', '........'],
        ['........', '........', '........', '.....1..', '....121.', '.....1..', '........', '........'],
        ['........', '.111....', '12221...', '.111....', '........', '........', '....11..', '........'],
        ['........', '........', '..3.....', '........', '........', '........', '......2.', '........'],
    ]
    for m in marks:
        tiles.append([[int(c) if c != '.' else 0 for c in row] for row in m])
    return tiles


# Hash value -> base tile. Mostly flat sky with occasional details.
BASE_PICK = [0] * 32
for value, tile in ((3, 1), (11, 2), (17, 3), (22, 4), (27, 5), (30, 6), (7, 7)):
    BASE_PICK[value] = tile


class Sector:
    def __init__(self, index, name, lab, palettes, stamps, plan, seed):
        self.index, self.name, self.lab = index, name, lab
        self.palettes = palettes       # six lists of four colours
        self.stamps = stamps           # list of (name, Pic)
        self.plan = plan               # list of (rows, density, {stamp: weight})
        self.seed = seed

    def build(self):
        sheet = BgSheet(self.palettes, base=0, limit=256)
        # Base tiles first, palette 0, never flipped variants needed.
        self.base = []
        for t in base_tiles(self.palettes[0]):
            n, flip = sheet.add(t)
            self.base.append((n, flip | 0))
        self.stamp_cells = []
        for name, pic in self.stamps:
            assert pic.w % 8 == 0 and pic.h % 8 == 0, name
            rows = sheet.map(pic, fill=self.palettes[0][0])
            self.stamp_cells.append((name, pic.w // 8, pic.h // 8, rows))
        self.sheet = sheet
        self.placements = self.layout()
        return self

    def layout(self):
        rnd = random.Random(self.seed)
        occupied = set()
        out = []
        sizes = {i: (w, h) for i, (_, w, h, _) in enumerate(self.stamp_cells)}
        names = {name: i for i, (name, _, _, _) in enumerate(self.stamp_cells)}

        def free(col, row, w, h):
            return all((c, r) not in occupied for c in range(col - 1, col + w + 1) for r in range(row - 1, row + h + 1))

        def place(col, row, sid, flip):
            w, h = sizes[sid]
            for c in range(col, col + w):
                for r in range(row, row + h):
                    occupied.add((c, r))
            out.append((row, col, sid, flip))

        row = 0
        for length, density, weights, rules in self.plan:
            end = row + length
            pool = [(names[k], v) for k, v in weights.items()]
            total = sum(v for _, v in pool)
            attempts = int(length * WIDTH * density / 40)
            for _ in range(attempts):
                pick = rnd.uniform(0, total)
                for sid, wgt in pool:
                    pick -= wgt
                    if pick <= 0:
                        break
                w, h = sizes[sid]
                lo, hi = rules.get('cols', (0, WIDTH))
                if hi - w < lo:
                    continue
                col = rnd.randint(lo, hi - w)
                r = rnd.randint(row, max(row, end - h))
                if r + h > end and length > h:
                    continue
                if free(col, r, w, h):
                    place(col, r, sid, rnd.random() < 0.5 and self.stamps[sid][0].startswith(('cloud', 'wisp')))
            row = end
        assert row <= ROWS, (self.name, row)
        out.sort()
        return out

    def tiles_used(self):
        return len(self.sheet.tiles)

    def cell(self, sid, x, y):
        """(sheet index, attribute) of a stamp cell in cartridge terms."""
        n, flip, pal = self.stamp_cells[sid][3][y][x]
        return n, pal | flip | (0x08 if n >= 128 else 0)

    def compose(self):
        """Render the level from exported data exactly as the cartridge will."""
        rows = ROWS + ARENA
        grid = [[None] * WIDTH for _ in range(rows)]
        for r in range(rows):
            for c in range(WIDTH):
                t = self.base[BASE_PICK[base_hash(c, r)]] if r < ROWS else self.base[0]
                grid[r][c] = (t[0], t[1])
        for (row, col, sid, flip) in self.placements:
            _, w, h, cells = self.stamp_cells[sid]
            for y in range(h):
                lr = row + h - 1 - y
                for x in range(w):
                    sx = (w - 1 - x) if flip else x
                    n, attr = self.cell(sid, sx, y)
                    if flip:
                        attr ^= 0x20
                    grid[lr][col + x] = (n, attr)
        return grid

    def preview(self, scale=1):
        grid = self.compose()
        rows = len(grid)
        im = Image.new('RGB', (WIDTH * 8, rows * 8))
        tiles = self.sheet.tiles
        for r in range(rows):
            for c in range(WIDTH):
                n, attr = grid[r][c]
                pal = self.palettes[attr & 7]
                t = tiles[n]
                for y in range(8):
                    for x in range(8):
                        sx = 7 - x if attr & 0x20 else x
                        sy = 7 - y if attr & 0x40 else y
                        im.putpixel((c * 8 + x, (rows - 1 - r) * 8 + y), hexrgb(pal[t[sy * 8 + sx]]))
        if scale != 1:
            im = im.resize((im.width * scale, im.height * scale), Image.Resampling.NEAREST)
        return im


# ----------------------------------------------------------- definitions

def sector1():
    base = '#2a58c4'
    pal = [
        [base, '#3a6cd6', '#5a8ee8', '#9cc8fc'],          # 0 deep sky
        [base, '#7aa8e8', '#d4ecff', '#ffffff'],          # 1 cloud
        [base, '#1c2a3a', '#3caa48', '#9ae05a'],          # 2 grass
        [base, '#1c2a3a', '#8a5232', '#d89a5a'],          # 3 rock
        ['#3caa48', '#1c2a3a', '#8a5232', '#9ae05a'],     # 4 grass over rock
        [base, '#0a1030', '#a8e4ff', '#ffffff'],          # 5 text, water
    ]
    cc = {'shadow': '#7aa8e8', 'mid': '#d4ecff', 'light': '#ffffff'}
    ic = {'dark': '#1c2a3a', 'top': '#3caa48', 'toplight': '#9ae05a', 'rock': '#8a5232', 'rocklight': '#d89a5a'}
    stamps = [
        ('cloud_big', T.cloud(64, 40, 11, cc)),
        ('cloud_mid', T.cloud(40, 24, 12, cc)),
        ('cloud_small', T.cloud(24, 16, 13, cc)),
        ('wisp', T.wisp(40, 8, 14, cc)),
        ('isle_big', T.island(72, 64, 15, ic)),
        ('isle_mid', T.island(48, 48, 16, ic)),
        ('isle_small', T.island(32, 32, 17, ic, cliff=8)),
    ]
    plan = [
        (OPEN_ROWS, 0.6, {'cloud_small': 2, 'wisp': 2}, {'cols': (0, 24)}),
        (60, 1.0, {'cloud_big': 1, 'cloud_mid': 2, 'cloud_small': 2, 'wisp': 2, 'isle_small': 1}, {}),
        (80, 1.2, {'isle_big': 1, 'isle_mid': 2, 'isle_small': 2, 'cloud_small': 1, 'wisp': 1}, {}),
        (70, 1.1, {'cloud_big': 2, 'cloud_mid': 2, 'isle_mid': 1, 'wisp': 1}, {}),
        (60, 1.0, {'isle_big': 1, 'isle_mid': 1, 'isle_small': 2, 'cloud_mid': 1, 'wisp': 2}, {}),
        (12, 0.5, {'wisp': 1}, {}),
    ]
    return Sector(0, 'AZURE ISLES', 'GROK', pal, stamps, plan, 101)


def sector2():
    base = '#5a3a8a'
    dark = '#3a1430'
    pal = [
        [base, '#6a4a9a', '#8a5aa8', '#c08ac8'],
        [base, '#a85a8a', '#ff9a8a', '#ffd8b0'],
        [base, dark, '#e8904a', '#ffc078'],
        [base, dark, '#b04a3a', '#e87a4a'],
        ['#e8904a', dark, '#b04a3a', '#ffc078'],
        [base, '#1a0a2a', '#ffd8b0', '#ffffff'],
    ]
    cc = {'shadow': '#a85a8a', 'mid': '#ff9a8a', 'light': '#ffd8b0'}
    ic = {'dark': dark, 'top': '#e8904a', 'toplight': '#ffc078', 'rock': '#b04a3a', 'rocklight': '#e87a4a'}
    stamps = [
        ('cloud_big', T.cloud(64, 40, 21, cc)),
        ('cloud_mid', T.cloud(48, 24, 22, cc)),
        ('cloud_small', T.cloud(24, 16, 23, cc)),
        ('wisp', T.wisp(48, 8, 24, cc)),
        ('mesa_big', T.island(72, 56, 25, ic, deco='rocks', cliff=12)),
        ('mesa_mid', T.island(48, 40, 26, ic, deco='rocks', cliff=10)),
        ('mesa_small', T.island(32, 32, 27, ic, deco='rocks', cliff=9)),
    ]
    plan = [
        (OPEN_ROWS, 0.6, {'cloud_small': 2, 'wisp': 2}, {}),
        (70, 1.2, {'cloud_big': 2, 'cloud_mid': 2, 'wisp': 2, 'mesa_small': 1}, {}),
        (80, 1.2, {'mesa_big': 1, 'mesa_mid': 2, 'mesa_small': 2, 'wisp': 1}, {}),
        (70, 1.2, {'cloud_big': 2, 'cloud_mid': 1, 'mesa_mid': 1, 'wisp': 2}, {}),
        (50, 1.0, {'mesa_big': 1, 'mesa_small': 2, 'cloud_small': 1, 'wisp': 2}, {}),
        (12, 0.5, {'wisp': 1}, {}),
    ]
    return Sector(1, 'CORAL DRIFT', 'CLAUDE', pal, stamps, plan, 202)


def sector3():
    base = '#14204a'
    dark = '#0a1030'
    pal = [
        [base, '#1e2e6a', '#3a5aa0', '#a0c0ff'],
        [base, '#1e2e6a', '#2e4a8a', '#5a7ac0'],
        [base, dark, '#5a6a9a', '#9aaad0'],
        [base, dark, '#3a4a7a', '#5cf2ee'],
        ['#3a4a7a', dark, '#ffe070', '#9aaad0'],
        [base, '#05081a', '#a0c0ff', '#ffffff'],
    ]
    cc = {'shadow': '#1e2e6a', 'mid': '#2e4a8a', 'light': '#5a7ac0'}
    city = {'dark': dark, 'deck': '#5a6a9a', 'decklight': '#9aaad0', 'facade': '#3a4a7a', 'window': '#ffe070',
            'glow': '#5cf2ee', 'roof': '#5a6a9a', 'hull': '#3a4a7a'}
    stamps = [
        ('cloud_big', T.cloud(64, 32, 31, cc)),
        ('cloud_mid', T.cloud(40, 24, 32, cc)),
        ('wisp', T.wisp(48, 8, 33, cc)),
        ('city_big', T.city(80, 56, 34, city)),
        ('city_mid', T.city(56, 40, 35, city)),
        ('city_small', T.city(32, 32, 36, city)),
    ]
    plan = [
        (OPEN_ROWS, 0.6, {'wisp': 2, 'cloud_mid': 1}, {}),
        (60, 1.1, {'city_small': 2, 'city_mid': 1, 'cloud_mid': 2, 'wisp': 2}, {}),
        (90, 1.3, {'city_big': 1, 'city_mid': 2, 'city_small': 2, 'wisp': 1}, {}),
        (60, 1.1, {'cloud_big': 2, 'cloud_mid': 2, 'city_small': 1, 'wisp': 1}, {}),
        (60, 1.2, {'city_big': 1, 'city_mid': 2, 'city_small': 1, 'wisp': 2}, {}),
        (12, 0.5, {'wisp': 1}, {}),
    ]
    return Sector(2, 'CRYSTAL CITY', 'GEMINI', pal, stamps, plan, 303)


def sector4():
    base = '#1e0f3e'
    dark = '#0a0418'
    pal = [
        [base, '#2e1a5a', '#6a4ab0', '#e0c0ff'],
        [base, '#2e1a5a', '#4a2a8a', '#8a5ad8'],
        [base, dark, '#3a2a6a', '#6a4ab0'],
        [base, dark, '#2a1a4a', '#ff5ac8'],
        ['#3a2a6a', dark, '#7af0ff', '#ffffff'],
        [base, dark, '#e0c0ff', '#ffffff'],
    ]
    cc = {'shadow': '#2e1a5a', 'mid': '#4a2a8a', 'light': '#8a5ad8'}
    ic = {'dark': dark, 'top': '#3a2a6a', 'toplight': '#6a4ab0', 'rock': '#2a1a4a', 'rocklight': '#ff5ac8',
          'crystal': '#7af0ff', 'crystallight': '#ffffff', 'glow': '#ff5ac8'}
    stamps = [
        ('nebula_big', T.cloud(64, 40, 41, cc)),
        ('nebula_mid', T.cloud(40, 32, 42, cc)),
        ('wisp', T.wisp(48, 8, 43, cc)),
        ('shard_big', T.island(72, 56, 44, ic, deco='crystals', cliff=12)),
        ('shard_mid', T.island(48, 48, 45, ic, deco='crystals', cliff=10)),
        ('shard_small', T.island(32, 32, 46, ic, deco='crystals', cliff=9)),
    ]
    plan = [
        (OPEN_ROWS, 0.6, {'wisp': 2, 'nebula_mid': 1}, {}),
        (70, 1.2, {'nebula_big': 2, 'nebula_mid': 2, 'wisp': 2, 'shard_small': 1}, {}),
        (80, 1.2, {'shard_big': 1, 'shard_mid': 2, 'shard_small': 2, 'wisp': 1}, {}),
        (60, 1.2, {'nebula_big': 1, 'nebula_mid': 2, 'shard_mid': 1, 'wisp': 2}, {}),
        (60, 1.2, {'shard_big': 1, 'shard_mid': 1, 'shard_small': 2, 'nebula_mid': 1}, {}),
        (12, 0.5, {'wisp': 1}, {}),
    ]
    return Sector(3, 'VIOLET NEXUS', 'MUSE', pal, stamps, plan, 404)


SECTORS = [sector1, sector2, sector3, sector4]
