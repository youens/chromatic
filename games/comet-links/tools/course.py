"""Comet Links course compiler and reference physics.

Holes are drawn below as 20 x 15 character grids that cover screen rows 2-16.
The compiler folds every planet, black hole, cup and wind region into one
force vector per 8 x 8 tile, so the cartridge only adds two numbers per
physics step. The same integer physics runs here for course checks and the
test bot; keep simulate() identical to step() in ../src/main.c.

Legend:  # asteroid   ~ nebula (heavy drag)   o bumper   T tee   U cup
         P planet (top-left of a 4 x 4 disc)  m moon (top-left of a 2 x 2)
         X black hole   > < ^ v solar wind (sets the tile's push)
"""
from collections import deque
from pathlib import Path
import math
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'shared'))
import worlds  # noqa: E402

T = worlds.COMET_TILES

# name, par, planet masses, grid
HOLES = [
 ('FIRST LIGHT', 2, {}, [
  '####################',
  '#..................#',
  '#..................#',
  '#.....~~~~~~.......#',
  '#.....~~~~~~.......#',
  '#..................#',
  '#.........#........#',
  '#.T......###.....U.#',
  '#.........#........#',
  '#..................#',
  '#.....~~~~~~.......#',
  '#.....~~~~~~.......#',
  '#..................#',
  '#..................#',
  '####################']),
 ('MOON BEND', 3, {'P': 4}, [
  '####################',
  '#..................#',
  '#..................#',
  '#..................#',
  '#..................#',
  '#.......P..........#',
  '#..................#',
  '#.T..............U.#',
  '#..................#',
  '#..................#',
  '#..................#',
  '#..................#',
  '#..................#',
  '#..................#',
  '####################']),
 ('SOLAR WIND', 3, {}, [
  '####################',
  '#..........^^^^#..U#',
  '#..........^^^^#...#',
  '#..........^^^^#...#',
  '#..........^^^^#...#',
  '#..........^^^^#...#',
  '#..........^^^^#...#',
  '#..........^^^^....#',
  '#..........^^^^....#',
  '#..........^^^^#####',
  '#..........^^^^....#',
  '#..........^^^^....#',
  '#.T........^^^^....#',
  '#..........^^^^....#',
  '####################']),
 ('BUMPER ALLEY', 3, {}, [
  '####################',
  '#..................#',
  '#.T.....o..........#',
  '#..................#',
  '#...........o......#',
  '#..................#',
  '#######...##########',
  '#..................#',
  '#....o......o......#',
  '#..................#',
  '#.........o.....U..#',
  '#..................#',
  '#...o..........o...#',
  '#..................#',
  '####################']),
 ('TWIN MOONS', 3, {'m': 3}, [
  '####################',
  '#..................#',
  '#..................#',
  '#........m.........#',
  '#..................#',
  '#..................#',
  '#.T..........###...#',
  '#............#U#...#',
  '#............#.#...#',
  '#..................#',
  '#........m.........#',
  '#..................#',
  '#..................#',
  '#..................#',
  '####################']),
 ('EVENT HORIZON', 3, {'m': 2}, [
  '####################',
  '#..................#',
  '#..................#',
  '#.......m..........#',
  '#..................#',
  '#..................#',
  '#..................#',
  '#.T......X.......U.#',
  '#..................#',
  '#..................#',
  '#..................#',
  '#.......m..........#',
  '#..................#',
  '#..................#',
  '####################']),
 ('THE LONG DRIFT', 5, {}, [
  '####################',
  '#.T................#',
  '#..................#',
  '#..................#',
  '#.............~~~~.#',
  '##############~~~~.#',
  '#.............~~~~.#',
  '#..................#',
  '#..................#',
  '#.............~~~~.#',
  '##############~~~~.#',
  '#.............~~~~.#',
  '#..................#',
  '#.U................#',
  '####################']),
 ('BINARY STAR', 4, {'P': 3}, [
  '####################',
  '#..................#',
  '#......P...........#',
  '#..................#',
  '#..................#',
  '#..................#',
  '#...........####...#',
  '#.T.........#U.<<<.#',
  '#...........####...#',
  '#..................#',
  '#..........P.......#',
  '#..................#',
  '#..................#',
  '#..................#',
  '####################']),
 ('WORMHOLE HOME', 5, {'P': 3, 'm': 2}, [
  '####################',
  '#..............#.U.#',
  '#..............#...#',
  '#....m.........#...#',
  '#..............o...#',
  '#.........>>>>>>...#',
  '#.........>>>>>>...#',
  '#.....######.......#',
  '#..................#',
  '#..........X.......#',
  '#..P...............#',
  '#..................#',
  '#..................#',
  '#.T.......~~~~~....#',
  '####################']),
]

SPACE, ROCK, NEBULA, BUMPER, PLANET, CUP, VOID, WIND = range(8)
SOLID = (ROCK, BUMPER, PLANET)
G_PLANET, G_VOID, G_CUP, G_WIND = 96, 1400, 360, 5
WIND = WIND
DIRX = [round(48 * math.cos(2 * math.pi * a / 64)) for a in range(64)]
DIRY = [round(48 * math.sin(2 * math.pi * a / 64)) for a in range(64)]
PUSH = {'>': (1, 0), '<': (-1, 0), '^': (0, -1), 'v': (0, 1)}


def compile_hole(index):
    name, par, masses, rows = HOLES[index]
    assert len(rows) == 15 and all(len(r) == 20 for r in rows), name
    # The cartridge skips playfield bounds checks, relying on this border.
    assert rows[0] == rows[14] == '#' * 20 and all(r[0] == r[19] == '#' for r in rows), name
    tiles = [0] * 300
    kinds = [SPACE] * 300
    wells = []
    tee = cup = void = None
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            n = y * 20 + x
            if ch == '#':
                tiles[n] = T['rock'][(x * 5 + y * 7 + x * y) % 3]
                kinds[n] = ROCK
            elif ch == '~':
                tiles[n] = T['nebula'][(x + y) % 2]
                kinds[n] = NEBULA
            elif ch == 'o':
                tiles[n] = T['bumper']
                kinds[n] = BUMPER
            elif ch in PUSH:
                tiles[n] = T['wind'][ch]
                kinds[n] = WIND
            elif ch == 'T':
                tee = (x * 8 + 4, y * 8 + 20)
                tiles[n] = T['tee']
            elif ch == 'U':
                cup = (x * 8 + 4, y * 8 + 20)
                tiles[n] = T['cup']
                kinds[n] = CUP
            elif ch == 'X':
                void = (x * 8 + 4, y * 8 + 20)
                tiles[n] = T['void']
                kinds[n] = VOID
                wells.append((void[0], void[1], G_VOID, 2))
            elif ch in 'Pm':
                size = 4 if ch == 'P' else 2
                wells.append((x * 8 + size * 4, y * 8 + 16 + size * 4, G_PLANET * masses[ch], 1))
                for dy in range(size):
                    for dx in range(size):
                        k = (y + dy) * 20 + x + dx
                        tiles[k] = (160 + dy * 4 + dx) if ch == 'P' else T['moon'][dy * 2 + dx]
                        corner = size == 4 and dx in (0, 3) and dy in (0, 3)
                        kinds[k] = SPACE if corner else PLANET
            else:
                assert ch == '.', (name, ch)
    assert tee and cup, name
    wells.append((cup[0], cup[1], G_CUP, 2))
    field = []
    for n in range(300):
        cx, cy = n % 20 * 8 + 4, n // 20 * 8 + 20
        ax = ay = 0.0
        for wx, wy, g, power in wells:
            dx, dy = wx - cx, wy - cy
            d2 = dx * dx + dy * dy
            if d2 < 16:
                continue
            scale = g / (d2 if power == 1 else d2 ** 1.5)
            ax += dx * scale
            ay += dy * scale
        ch = rows[n // 20][n % 20]
        if ch in PUSH:
            ax += PUSH[ch][0] * G_WIND
            ay += PUSH[ch][1] * G_WIND
        field.append((max(-127, min(127, round(ax))), max(-127, min(127, round(ay)))))
    dx, dy = cup[0] - tee[0], cup[1] - tee[1]
    aim = round(math.atan2(dy, dx) / (2 * math.pi) * 64) % 64
    return {'name': name, 'par': par, 'tiles': tiles, 'kinds': kinds, 'field': field,
            'tee': tee, 'cup': cup, 'void': void or (0, 0), 'aim': aim}


def drag(v, heavy):
    if v > 0:
        v -= (v >> 3) + 2 if heavy else (v >> 6) + 1
        return max(v, 0)
    if v < 0:
        m = -v
        m -= (m >> 3) + 2 if heavy else (m >> 6) + 1
        return -max(m, 0)
    return 0


def bounce(v, bumper):
    m = abs(v)
    m = min(900, m + (m >> 2) + 64) if bumper else (m * 3) >> 2
    return -m if v > 0 else m


def kind_at(hole, px, py):
    if px < 0 or px >= 160 or py < 16 or py >= 136:
        return ROCK
    return hole['kinds'][((py - 16) >> 3) * 20 + (px >> 3)]


SUNK, LOST, STOPPED = 1, 2, 3


def simulate(hole, x, y, vx, vy, trace=None, limit=1200):
    """Run one stroke from 1/256-pixel position (x, y); returns (event, x, y)."""
    kinds, field = hole['kinds'], hole['field']
    ux, uy = hole['cup']
    hx, hy = hole['void']
    still = 0
    for t in range(limit):
        px, py = x >> 8, y >> 8
        n = ((py - 16) >> 3) * 20 + (px >> 3)
        fx, fy = field[n]
        vx += fx
        vy += fy
        heavy = kinds[n] == NEBULA
        vx = max(-1024, min(1024, drag(vx, heavy)))
        vy = max(-1024, min(1024, drag(vy, heavy)))
        nx = (x + vx) & 0xFFFF
        probe = (nx >> 8) + (2 if vx > 0 else -2 if vx < 0 else 0)
        k = kind_at(hole, probe, py)
        if k in SOLID:
            vx = bounce(vx, k == BUMPER)
        else:
            x = nx
            px = x >> 8
        ny = (y + vy) & 0xFFFF
        probe = (ny >> 8) + (2 if vy > 0 else -2 if vy < 0 else 0)
        k = kind_at(hole, px, probe)
        if k in SOLID:
            vy = bounce(vy, k == BUMPER)
        else:
            y = ny
            py = y >> 8
        if trace is not None:
            trace.append((px, py))
        if abs(px - ux) < 4 and abs(py - uy) < 4 and abs(vx) + abs(vy) < 320:
            return SUNK, x, y
        if hx and abs(px - hx) < 4 and abs(py - hy) < 4:
            return LOST, x, y
        if abs(vx) + abs(vy) < 24:
            still += 1
            if still >= 10:
                return STOPPED, x, y
        else:
            still = 0
    return STOPPED, x, y


def launch(aim, power):
    return power * DIRX[aim], power * DIRY[aim]


def route_map(hole):
    """Tile distance to the cup through non-solid tiles."""
    dist = [999] * 300
    ux, uy = hole['cup']
    start = ((uy - 16) >> 3) * 20 + (ux >> 3)
    dist[start] = 0
    queue = deque([start])
    while queue:
        n = queue.popleft()
        for m in (n - 20, n + 20, n - 1 if n % 20 else -1, n + 1 if n % 20 < 19 else -1):
            if 0 <= m < 300 and dist[m] == 999 and hole['kinds'][m] not in SOLID:
                dist[m] = dist[n] + 1
                queue.append(m)
    return dist


def plan(hole, x, y, aims=range(64), powers=range(1, 17)):
    """Best (aim, power) by outcome: sink, then shortest remaining route."""
    route = route_map(hole)
    ux, uy = hole['cup']
    best = None
    for aim in aims:
        for power in powers:
            vx, vy = launch(aim, power)
            event, ex, ey = simulate(hole, x, y, vx, vy)
            if event == SUNK:
                cost = -1000 + power * 0.01
            elif event == LOST:
                cost = 5000
            else:
                px, py = ex >> 8, ey >> 8
                cost = route[((py - 16) >> 3) * 20 + (px >> 3)] * 8 + math.hypot(px - ux, py - uy) * 0.25
            if best is None or cost < best[0]:
                best = (cost, aim, power, event, ex, ey)
    return best


def play(hole, max_strokes=8):
    x, y = hole['tee'][0] << 8, hole['tee'][1] << 8
    shots = []
    for stroke in range(1, max_strokes + 1):
        cost, aim, power, event, ex, ey = plan(hole, x, y)
        shots.append((aim, power))
        if event == SUNK:
            return stroke, shots
        if event != LOST:
            x, y = ex, ey
    return None, shots


def c_array(kind, name, values):
    body = ''.join('    ' + ','.join(str(v) for v in values[i:i + 20]) + ',\n' for i in range(0, len(values), 20))
    return f'const {kind} {name}[] = {{\n{body}}};\n'


def header():
    holes = [compile_hole(i) for i in range(len(HOLES))]
    # tile_info packs each tile's collision kind with its palette (<< 4).
    info = [0] * 256
    for t in T['rock']:
        info[t] = ROCK | 6 << 4
    for t in T['nebula']:
        info[t] = NEBULA | 7 << 4
    for t in T['wind'].values():
        info[t] = WIND | 7 << 4
    for t in T['moon']:
        info[t] = PLANET | 5 << 4
    for i in range(16):
        info[160 + i] = (SPACE if i in (0, 3, 12, 15) else PLANET) | 5 << 4
    info[T['bumper']] = BUMPER | 1 << 4
    info[T['cup']] = CUP | 2 << 4
    info[T['void']] = VOID | 3 << 4
    info[T['tee']] = SPACE | 4 << 4
    out = '#include <stdint.h>\n'
    out += f'#define HOLE_COUNT {len(holes)}\n'
    out += c_array('uint8_t', 'course_tiles', sum((h['tiles'] for h in holes), []))
    out += c_array('int8_t', 'course_field', [v for h in holes for pair in h['field'] for v in pair])
    out += c_array('uint8_t', 'tile_info', info)
    for key, pick in [('tee_x', lambda h: h['tee'][0]), ('tee_y', lambda h: h['tee'][1]),
                      ('cup_x', lambda h: h['cup'][0]), ('cup_y', lambda h: h['cup'][1]),
                      ('void_x', lambda h: h['void'][0]), ('void_y', lambda h: h['void'][1]),
                      ('hole_par', lambda h: h['par']), ('hole_aim', lambda h: h['aim'])]:
        out += c_array('uint8_t', key, [pick(h) for h in holes])
    out += 'const char *const hole_names[] = {' + ','.join(f'"{i + 1} {h["name"]}"' for i, h in enumerate(holes)) + '};\n'
    out += c_array('int8_t', 'dir_x', DIRX) + c_array('int8_t', 'dir_y', DIRY)
    return out


if __name__ == '__main__':
    if sys.argv[1] == '--check':
        total = 0
        for i in range(len(HOLES)):
            hole = compile_hole(i)
            strokes, shots = play(hole)
            total += strokes or 99
            print(f"{i + 1} {hole['name']:<14} par {hole['par']} bot {strokes} {shots}", flush=True)
        print('bot total', total, 'par', sum(h[1] for h in HOLES))
    else:
        Path(sys.argv[1]).write_text(header())
