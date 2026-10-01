"""HUD, panel and icon background tiles.

The HUD occupies the window's two tile rows. Row-0 tiles carry a one-pixel
border on top so the panel reads as a solid console below the sky.
"""
from font import glyph_rows, GLYPHS


def grid(rows):
    return [[0 if c == '.' else int(c) for c in row] for row in rows]


def blank(v=0):
    return [[v] * 8 for _ in range(8)]


def bordered(cell):
    """Shift ink down one pixel and draw the top border in colour 1."""
    out = [[1] * 8] + [row[:] for row in cell[:7]]
    return out


def plain_glyph(ch, colour=3):
    rows = GLYPHS[ch]
    cell = blank()
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            if c == '#':
                cell[y][x] = colour
    return cell


HEART_FULL = grid(['........', '.22.22..', '2222222.', '2222222.', '2222222.', '.22222..', '..222...', '...2....'])
HEART_EMPTY = grid(['........', '.11.11..', '1..1..1.', '1.....1.', '1.....1.', '.1...1..', '..1.1...', '...1....'])
BOLT = grid(['....22..', '...232..', '..232...', '.22332..', '..2332..', '..232...', '.232....', '.22.....'])
COIN = grid(['..222...', '.23332..', '2332232.', '2323232.', '2323232.', '2322232.', '.22222..', '..222...'])
POWER = grid(['.222222.', '23333332', '23222232', '23222232', '23333322', '23222222', '23222222', '.222222.'])
BOSS = grid(['..2222..', '.233332.', '23333332', '23122132', '23333332', '.232232.', '..2..2..', '........'])
CHAIN_X = grid(['........', '........', '.33..33.', '..3333..', '...33...', '..3333..', '.33..33.', '........'])


def bar(level, fill=3):
    """A gauge segment filled `level` pixels from the left."""
    cell = blank()
    for x in range(8):
        cell[1][x] = 1
        cell[6][x] = 1
    for y in range(2, 6):
        for x in range(8):
            cell[y][x] = fill if x < level else 1
    return cell


def hud_tiles():
    """Named HUD tiles, in VRAM order starting at tile 0."""
    tiles = [('BLANK', blank()), ('EDGE', bordered(blank()))]
    for d in '0123456789':
        tiles.append(('D' + d, bordered(plain_glyph(d))))
    tiles += [
        ('HEART', bordered(HEART_FULL)), ('HEART_OFF', bordered(HEART_EMPTY)),
        ('CHAIN', bordered(CHAIN_X)),
    ]
    tiles += [('BAR%d' % i, bar(i)) for i in range(9)]
    tiles += [('BOLT', BOLT), ('COIN', COIN), ('POWER', POWER), ('BOSS', BOSS)]
    return tiles


def boss_bar_tiles():
    """Boss health segments in colour 2, at two-pixel steps."""
    return [('BBAR%d' % i, bar(i, 2)) for i in (0, 2, 4, 6, 8)]


# --------------------------------------------------------------- panels
# Colours: 0 backdrop, 1 dark rim, 2 light border, 3 panel fill.
PANEL = {
    'TL': ['..1111..'[:0] + '...11111', '..122222', '.1233333', '1233333 3'.replace(' ', ''), '12333333', '12333333', '12333333', '12333333'],
    'T': ['11111111', '22222222', '33333333', '33333333', '33333333', '33333333', '33333333', '33333333'],
    'L': ['12333333'] * 8,
    'C': ['33333333'] * 8,
    'BL': ['12333333', '12333333', '12333333', '12333333', '1233333 3'.replace(' ', ''), '.1233333', '..122222', '...11111'],
    'B': ['33333333', '33333333', '33333333', '33333333', '33333333', '33333333', '22222222', '11111111'],
    'DIV': ['33333333', '33333333', '33333333', '22222222', '33333333', '33333333', '33333333', '33333333'],
}


def panel_tiles():
    out = []
    for name in ('TL', 'T', 'L', 'C', 'BL', 'B', 'DIV'):
        out.append((name, grid(PANEL[name])))
    return out
