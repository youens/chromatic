"""Play Dreambase Invaders in PyBoy with real joypad input.

A bot reads projectile positions from emulator memory and steers the analyst
with the D-pad, so the full eight-level campaign is played through the
game's own controls. Memory is written only to set up the game-over check.
Screenshots go to art/ and results to build/validation.json.
"""
from pathlib import Path
import hashlib
import json
import re

from pyboy import PyBoy

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'
ART = ROOT / 'art'
ROM = BUILD / 'dreambase-invaders.gbc'
SYM = {k: int(v, 16) for k, v in re.findall(r'DEF (_\w+) 0x([\da-fA-F]+)', (BUILD / 'dreambase-invaders.noi').read_text())}
p = PyBoy(str(ROM), window='null', sound_emulated=False)
p.set_emulation_speed(0)
held = set()
checks = []


def run(n=1, keys=()):
    global held
    keys = set(keys)
    for k in held - keys:
        p.button_release(k)
    for k in keys - held:
        p.button_press(k)
    held = keys
    p.tick(n)


def get(name, offset=0):
    return p.memory[SYM['_' + name] + offset]


def put(name, value, offset=0):
    p.memory[SYM['_' + name] + offset] = value


def word(name, offset=0, signed=True):
    v = get(name, offset) + 256 * get(name, offset + 1)
    return v - 65536 if signed and v >= 32768 else v


def press(key, settle=30):
    run(4, [key])
    run(settle)


def shot(name):
    p.screen.image.save(ART / f'{name}.png')


def projectiles():
    out = []
    for i in range(12):
        if get('pr_on', i):
            out.append((word('pr_x', 2 * i) / 64, word('pr_y', 2 * i) / 64, -word('pr_vy', 2 * i) / 64))
    return out


def bot_keys():
    """Chase the most urgent logo that can still be reached."""
    px, py = word('pl_x') / 64, word('pl_y') / 64
    best = None
    for x, y, v in projectiles():
        if y > 100 or v <= 0:
            continue
        escape = (y + 8) / v
        tx, ty = x, max(10, min(76, y - v * 5))
        reach = max(abs(tx - px), abs(ty - py)) / 1.5
        if reach < escape and (best is None or escape < best[0]):
            best = (escape, tx, ty)
    tx, ty = (80, 40) if best is None else best[1:]
    keys = []
    if tx - px > 2:
        keys.append('right')
    elif px - tx > 2:
        keys.append('left')
    if ty - py > 2:
        keys.append('down')
    elif py - ty > 2:
        keys.append('up')
    if best and max(abs(tx - px), abs(ty - py)) > 36:
        keys.append('a')
    return keys


try:
    data = ROM.read_bytes()
    assert len(data) == 32768 and data[0x143] == 0xC0 and data[0x147] == 0
    assert data[0x14D] == (-sum(data[0x134:0x14D]) - 25) & 255
    assert int.from_bytes(data[0x14E:0x150], 'big') == (sum(data) - sum(data[0x14E:0x150])) & 0xFFFF
    checks.append('GBC header and checksums')

    # Brand splash: the mark draws itself, then the wordmark and domain.
    run(250)
    assert get('db_mode') == 0
    shot('splash')
    run(20)
    assert get('db_mode') == 0
    press('a')
    run(30)
    assert get('db_mode') == 1, 'title'
    run(100)
    shot('title')
    # The formation keeps marching between the screen edges.
    xs = set()
    for _ in range(1500):
        run(1)
        xs.add(p.memory[0xC001])  # shadow OAM: the first formation sprite's x
    assert min(xs) >= 8 and max(xs) <= 56 and len(xs) > 6, ('formation march', sorted(xs))
    checks.append('brand splash, title and a 25-second formation march')

    # Difficulty selector.
    assert get('db_diff') == 1
    press('left', 10)
    assert get('db_diff') == 0
    press('right', 10)
    press('right', 10)
    assert get('db_diff') == 2
    press('left', 10)
    assert get('db_diff') == 1
    checks.append('difficulty selector')

    # How to play, then the data stack page.
    press('b')
    assert get('db_mode') == 2
    shot('help')
    press('a')
    shot('stack')
    press('b')
    press('b')
    assert get('db_mode') == 1
    checks.append('two help pages')

    press('start', 10)
    for _ in range(120):
        if get('db_mode') == 3:
            break
        run(1)
    run(20)
    assert get('db_mode') == 3 and get('pl_phase') == 3, 'level 1 banner'
    shot('intro')
    for _ in range(200):
        if get('pl_phase') == 0:
            break
        run(1)
    assert get('pl_phase') == 0 and get('db_level') == 0
    # 60 Hz movement and the dash.
    x = word('pl_x')
    run(30, ['right'])
    walked = word('pl_x') - x
    assert 60 * 64 // 2 <= walked * 2 <= 100 * 64, ('walk speed', walked)
    x = word('pl_x')
    run(1, ['left', 'a'])
    run(8, ['left'])
    dashed = x - word('pl_x')
    assert dashed > 2 * 9 * 96, ('dash', dashed)
    checks.append('movement and dash')

    # Pause freezes the logos and the analyst.
    run(120)
    press('start', 10)
    assert get('pl_paused') == 1
    before = (projectiles(), word('pl_x'))
    run(90, ['up'])
    assert (projectiles(), word('pl_x')) == before, 'pause'
    press('start', 10)
    assert get('pl_paused') == 0
    checks.append('pause and resume')

    # A miss drains the life bar and breaks the streak.
    life = get('pl_life')
    for _ in range(900):
        run(1, ['left', 'up'] if word('pl_x') > 12 * 64 else ['up'])
        if get('pl_life') < life:
            break
    assert get('pl_life') < life and get('pl_streak') == 0, 'miss drains life'
    checks.append('missed logos drain life')

    # Play the whole campaign through the joypad.
    levels, shots, frames, caught = [], set(), 0, 0
    last_catches = get('pl_catches')
    play_frames = play_updates = 0
    while get('db_mode') == 3 and frames < 60 * 60 * 10:
        # Only the low byte: the emulator can pause between the two halves
        # of the 16-bit increment.
        before = (get('pl_phase'), get('db_frame'))
        run(1, bot_keys())
        frames += 1
        if before[0] == 0 and get('pl_phase') == 0:
            play_frames += 1
            play_updates += (get('db_frame') - before[1]) & 0xFF
        level, phase, timer = get('db_level'), get('pl_phase'), word('pl_timer', signed=False)
        if get('pl_catches') > last_catches:
            caught += get('pl_catches') - last_catches
        last_catches = get('pl_catches')
        if not levels or levels[-1][0] != level:
            levels.append((level, frames, get('pl_lives')))
        if level == 3 and phase == 0 and sum(1 for _, y, _ in projectiles() if 8 < y < 90) >= 3 and get('pp_life') and 'gameplay' not in shots:
            shot('gameplay')
            shots.add('gameplay')
        if level == 6 and phase == 0 and sum(1 for _, y, _ in projectiles() if 8 < y < 90) >= 4 and 'late' not in shots:
            shot('late-level')
            shots.add('late')
        if phase == 1 and timer == 120 and level == 3 and 'levelup' not in shots:
            shot('levelup')
            shots.add('levelup')
    rate = play_updates / play_frames
    assert rate > 0.99, ('update rate during play', rate)
    assert [lv for lv, _, _ in levels] == list(range(8)), levels
    assert get('db_mode') == 5 and get('db_won') == 1, ('credits', get('db_mode'))
    run(400)
    shot('credits')
    for _ in range(3000):
        if get('db_mode') == 4:
            break
        run(1)
    assert get('db_mode') == 4, 'results after credits'
    run(60)
    shot('results')
    score = word('db_score', signed=False)
    assert word('db_best', signed=False) == score > 0
    checks.append('full eight-level campaign, transformations, credits')

    # Game over: one life with an almost empty bar, then let a logo escape.
    press('a', 150)
    assert get('db_mode') == 3 and get('pl_phase') == 0 and word('db_score', signed=False) == 0
    put('pl_lives', 1)
    put('pl_life', 1)
    for _ in range(1200):
        run(1, ['left', 'up'])
        if get('db_mode') == 4:
            break
    assert get('db_mode') == 4 and get('db_won') == 0, 'game over'
    run(60)
    shot('game-over')
    assert word('db_best', signed=False) == score, 'best score kept'
    checks.append('game over keeps the best score')

    (BUILD / 'validation.json').write_text(json.dumps({
        'sha256': hashlib.sha256(data).hexdigest(),
        'passed': checks,
        'campaign': {'difficulty': 'normal', 'frames': frames, 'score': score,
                     'updates_per_frame': round(rate, 4),
                     'levels': [{'level': lv + 1, 'frame': f, 'lives': l} for lv, f, l in levels]},
    }, indent=2) + '\n')
    print('PASS:', ', '.join(checks), f'(campaign {frames} frames, {rate:.3f} updates per frame in play, score {score})')
finally:
    p.stop(save=False)
