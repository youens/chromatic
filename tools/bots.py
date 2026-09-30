"""Joypad-driven players for the newer cartridges.

Each bot reads exported game state from emulator memory to choose inputs,
then acts only through the joypad. Like the older maze and puzzle solvers,
they see information a human cannot (hidden bolts' paths, the exact physics,
the next placement's outcome), so they prove the games are completable rather
than measuring human difficulty.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'games/comet-links/tools'))
import course  # noqa: E402


def s8(v):
    return v - 256 if v > 127 else v


def stormkite(c, frames=9000, snapshot=None):
    """Hold fire, pick the least threatened altitude, spend gusts on danger."""
    for frame in range(frames):
        if c.get('phase') != 2:
            break
        if frame % 2:
            c.run(1, c.held)
            continue
        kx, ky, wave, boss = c.get('kx'), c.get('ky'), c.get('wave'), c.get('boss')
        threats = []
        for i in range(5):
            if c.get('bolt_x', i):
                threats.append((c.get('bolt_x', i), c.get('bolt_y', i), -3, s8(c.get('bolt_dy', i)), 4))
        targets = []
        for i in range(5):
            kind = c.get('foe', i)
            if kind:
                size = 16 if kind == 2 else 8
                vx = {1: -2 if wave >= 3 else -1, 2: -2, 3: -3}[kind]
                threats.append((c.get('foe_x', i), c.get('foe_y', i), vx, 0, size))
                if c.get('foe_x', i) > kx + 8:
                    targets.append(c.get('foe_y', i) + size // 2 - 4)
        if boss == 1:
            threats.append((c.get('boss_x'), c.get('boss_y'), 0, 0, 24))
            targets.append(c.get('boss_y') + 8)
        lantern = c.get('lantern_x')
        best = None
        for cy in range(18, 119, 2):
            if abs(cy - ky) > 24:
                continue
            cost = abs(cy - ky) * 0.05
            for tx, ty, vx, vy, size in threats:
                for t in range(0, 16, 2):
                    y = ky + max(-2 * t, min(2 * t, cy - ky))
                    if abs(tx + vx * t + size / 2 - kx - 8) < size / 2 + 8 and abs(ty + vy * t + size / 2 - y - 7) < size / 2 + 8:
                        cost += 40 / (1 + t * 0.3)
            if targets:
                cost += min(abs(cy + 4 - t) for t in targets) * 0.08
            if lantern and lantern > kx:
                cost += abs(cy + 4 - c.get('lantern_y')) * 0.05
            if best is None or cost < best[0]:
                best = (cost, cy)
        keys = ['a']
        if best[1] < ky - 1:
            keys.append('up')
        elif best[1] > ky + 1:
            keys.append('down')
        home = 20 if boss == 1 else 24
        if kx < home - 1:
            keys.append('right')
        elif kx > home + 1:
            keys.append('left')
        if c.get('gust') >= 24 and (best[0] > 30 or boss == 1) and frame % 4 == 0:
            keys.append('b')
        c.run(1, keys)
        if snapshot and frame == 3000:
            snapshot()


def comet_links(c, snapshot=None):
    """Plan every stroke with the reference physics, then aim with the D-pad."""
    holes = [course.compile_hole(i) for i in range(len(course.HOLES))]
    strokes = []
    mismatches = 0
    used_scope = False
    while c.get('phase') == 2:
        if c.get('shot_mode') != 1:
            c.run(2)
            continue
        hole = holes[c.get('hole')]
        x, y = c.word('bx'), c.word('by')
        _, aim, power, event, ex, ey = course.plan(hole, x, y)
        while c.get('aim') != aim:
            c.tap('right' if (aim - c.get('aim')) % 64 <= 32 else 'left')
        while c.get('power') != power:
            c.tap('up' if c.get('power') < power else 'down')
        if c.get('hole') == 2 and c.get('strokes') == 0 and c.get('scope'):
            before = c.get('scope')
            c.tap('b')
            c.run(90)
            assert c.get('scope_on') == 1 and c.get('scope') == before - 1, 'scope'
            assert c.get('dot_count') >= 8, 'scope preview'
            used_scope = True
            if snapshot:
                snapshot()
        c.tap('a')
        while c.get('shot_mode') == 2:
            c.run(2)
        if c.get('shot_mode') == 1 and event != course.LOST and (c.word('bx'), c.word('by')) != (ex, ey):
            mismatches += 1
        if c.get('shot_mode') == 3:
            strokes.append(c.get('strokes'))
    assert used_scope
    return {'strokes': strokes, 'par': sum(h[1] for h in course.HOLES), 'physics_mismatches': mismatches}


PRISM_W, PRISM_H = 7, 15
PRISM_DIRS = [(1, 0), (0, 1), (1, 1), (1, -1)]


def _prism_matches(b):
    found = set()
    for r in range(PRISM_H):
        for col in range(PRISM_W):
            v = b[r * PRISM_W + col]
            if not v or v >= 6:
                continue
            for dx, dy in PRISM_DIRS:
                pr, pc = r - dy, col - dx
                if 0 <= pr < PRISM_H and 0 <= pc < PRISM_W and b[pr * PRISM_W + pc] == v:
                    continue
                cells, rr, cc = [], r, col
                while 0 <= rr < PRISM_H and cc < PRISM_W and b[rr * PRISM_W + cc] == v:
                    cells.append(rr * PRISM_W + cc)
                    rr += dy
                    cc += dx
                if len(cells) >= 3:
                    found.update(cells)
    return found


def _prism_settle(b):
    for col in range(PRISM_W):
        stack = [b[r * PRISM_W + col] for r in range(PRISM_H) if b[r * PRISM_W + col]]
        stack = [0] * (PRISM_H - len(stack)) + stack
        for r in range(PRISM_H):
            b[r * PRISM_W + col] = stack[r]


def _prism_resolve(b, marked=None):
    gems = stones = chains = 0
    while True:
        found = marked if marked is not None else _prism_matches(b)
        marked = None
        if not found:
            return gems, stones, chains
        chains += 1
        for n in found:
            b[n] = 0
        gems += len(found)
        for n in found:
            r, col = divmod(n, PRISM_W)
            for k, ok in ((n - PRISM_W, r > 0), (n + PRISM_W, r < PRISM_H - 1), (n - 1, col > 0), (n + 1, col < PRISM_W - 1)):
                if ok and b[k] == 6:
                    b[k] = 0
                    stones += 1
        _prism_settle(b)


def _prism_score(board, piece, x):
    b = board[:]

    def fits(y):
        return all(y + i < PRISM_H and (y + i < 0 or not b[(y + i) * PRISM_W + x]) for i in range(3))
    y = -2
    if not fits(y):
        return None
    while fits(y + 1):
        y += 1
    if y < 0:
        return None
    for i in range(3):
        b[(y + i) * PRISM_W + x] = piece[i]
    marked = None
    if piece[0] == 7:
        below = b[(y + 3) * PRISM_W + x] if y + 3 < PRISM_H else 0
        marked = {(y + i) * PRISM_W + x for i in range(3)}
        if below and below < 6:
            marked |= {n for n, v in enumerate(b) if v == below}
    gems, stones, chains = _prism_resolve(b, marked)
    heights = []
    for col in range(PRISM_W):
        top = next((r for r in range(PRISM_H) if b[r * PRISM_W + col]), PRISM_H)
        heights.append(PRISM_H - top)
    pairs = 0
    for r in range(PRISM_H):
        for col in range(PRISM_W):
            v = b[r * PRISM_W + col]
            if v and v < 6:
                for dx, dy in PRISM_DIRS:
                    rr, cc = r + dy, col + dx
                    if 0 <= rr < PRISM_H and 0 <= cc < PRISM_W and b[rr * PRISM_W + cc] == v:
                        pairs += 1
    return (stones * 400 + gems * 10 + chains * 30 + pairs * 3 - max(heights) * 6 - sum(heights) * 1.5
            - (40 if heights[3] > 10 else 0) - sum(max(0, h - 9) * 20 for h in heights))


def prism_well(c, pieces=600, snapshot=None):
    """Search every column and colour order, then drive the piece there."""
    placed = 0
    while c.get('phase') == 2 and placed < pieces:
        if c.get('state') != 0 or s8(c.get('piece_y')) != -2:
            c.run(2)
            continue
        board = [c.get('board', i) for i in range(PRISM_W * PRISM_H)]
        piece = [c.get('piece', i) for i in range(3)]
        best = None
        for turns in range(3):
            order = piece[:]
            for _ in range(turns):
                order = [order[2], order[0], order[1]]
            for x in range(PRISM_W):
                score = _prism_score(board, order, x)
                if score is not None and (best is None or score > best[0]):
                    best = (score, turns, x)
        _, turns, x = best or (0, 0, 3)
        for _ in range(turns):
            c.tap('a')
        while c.get('piece_x') < x:
            c.tap('right')
        while c.get('piece_x') > x:
            c.tap('left')
        c.tap('up')
        placed += 1
        if snapshot and placed == 30:
            snapshot()
    return placed
