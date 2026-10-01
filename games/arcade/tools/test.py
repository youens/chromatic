"""Exercise the launcher and every game in one emulator session.

Covers the cartridge header, the launcher's intro, shelf, info and records
screens, the fixed header during a shelf slide, launching and returning from
every game twice, a real Neon Wake best score, save RAM surviving a power
cycle, erasing the records, and carrying both historical save layouts forward.
"""
from pathlib import Path
import hashlib
import io
import json
import re
from pyboy import PyBoy

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'
rom = BUILD / 'chromatic-arcade.gbc'
data = rom.read_bytes()
assert len(data) == 524288 and data[0x143] == 0xC0
assert data[0x147] == 0x1B and data[0x148] == 0x04 and data[0x149] == 0x02, 'MBC5, 512 KiB, battery RAM'
assert data[0x14D] == (-sum(data[0x134:0x14D]) - 25) & 255
assert int.from_bytes(data[0x14E:0x150], 'big') == (sum(data) - sum(data[0x14E:0x150])) & 65535
symbols = {k: int(v, 16) for k, v in re.findall(r'DEF (_\w+) 0x([\da-fA-F]+)', (BUILD / 'chromatic-arcade.noi').read_text())}
GAMES = ['neon-wake', 'moonthread', 'echo-vault', 'bloom-circuit', 'orbit-choir', 'hello-dot', 'stormkite', 'comet-links', 'prism-well', 'dot-swarm', 'dreambase-invaders', 'dotwing']
LAST = len(GAMES) - 1
HELLO = 5
p = None
held = set()


def boot(ram):
    global p, held
    p = PyBoy(str(rom), window='null', sound_emulated=False, ram_file=io.BytesIO(ram))
    p.set_emulation_speed(0)
    held = set()


def run(frames=30, buttons=()):
    global held
    buttons = set(buttons)
    for k in held - buttons:
        p.button_release(k)
    for k in buttons - held:
        p.button_press(k)
    held = buttons
    p.tick(frames)


def press(key):
    run(6, [key])
    run(20)
    settle()


def dot_press(key):
    """Let a complete native menu redraw finish before the next edge."""
    press(key)
    before = word('dw_clock')
    for _ in range(200):
        run(1)
        if (word('dw_clock') - before) & 65535 >= 2:
            return
    raise AssertionError('Dotwing menu redraw stalled')


def settle():
    # Wait for any launcher fade to finish before the next input.
    for _ in range(60):
        if get('arcade_in_game') or get('fade_level') == 8:
            return
        run(2)
    raise AssertionError('fade stalled')


def get(name, offset=0):
    return p.memory[symbols['_' + name] + offset]


def word(name, offset=0):
    return get(name, offset) + (get(name, offset + 1) << 8)


def best(i):
    return word('records', 6 + 2 * i)


def plays(i):
    return word('records', 6 + 2 * len(GAMES) + 2 * i)


def shot(name):
    p.screen.image.save(BUILD / f'{name}.png')


def signed(name, offset=0):
    v = word(name, offset)
    return v - 65536 if v >= 32768 else v


def historical_save(version, best, plays, last):
    """A real historical record, with independent SRAM beyond the launcher."""
    body = bytearray(b'CHRA') + bytes([version, last])
    for v in best + plays:
        body += v.to_bytes(2, 'little')
    s = 0x5A17
    for b in body:
        s = (((s << 1) | (s >> 15)) & 0xFFFF) ^ b
    body += s.to_bytes(2, 'little')
    ram = body + bytes([0xFF]) * (8192 - len(body))
    ram[0x200:0x220] = bytes(range(32))
    return bytes(ram)


saved = io.BytesIO()
results = []
pilot_choice = None
boot(bytes([0xFF]) * 8192)
try:
    run(120)
    assert get('launcher_mode') == 0 and get('arcade_in_game') == 0, 'intro'
    shot('launcher-intro')
    press('start')
    run(20)
    assert get('launcher_mode') == 1 and get('menu_index') == 0, 'shelf'
    shot('launcher-shelf')
    # Mid-slide, the header rows must not move while the icon band does.
    before = p.screen.ndarray.copy()
    run(3, ['right'])
    during = p.screen.ndarray.copy()
    assert (before[0:15] == during[0:15]).all(), 'header moved during slide'
    assert (before[24:72] != during[24:72]).any(), 'icon band did not slide'
    run(30)
    assert get('menu_index') == 1
    press('left')
    run(20)
    assert get('menu_index') == 0
    for cycle in range(2):
        for i, slug in enumerate(GAMES):
            assert get('menu_index') == i, (slug, get('menu_index'))
            press('a')
            run(10)
            assert get('arcade_in_game') == 1, (slug, 'launch')
            state = 'scene' if i == HELLO else 'phase'
            if slug == 'dotwing':
                for _ in range(200):
                    if word('dw_clock') >= 2:
                        break
                    run(1)
                assert get('dw_state') == 0, ('dotwing title', get('dw_state'))
                if cycle == 0:
                    shot('dotwing-title')
                dot_press('a')
                if get('dw_state') == 1:
                    dot_press('right')
                    dot_press('down')
                    dot_press('right')
                    shot('dotwing-builder')
                    dot_press('start')
                assert get('dw_state') == 2, ('dotwing hangar', get('dw_state'))
                choice = bytes(get('dw_profile', n) for n in range(4))
                if cycle == 0:
                    pilot_choice = choice
                else:
                    assert choice == pilot_choice, 'dotwing saved pilot survived launcher return'
                dot_press('b')
                assert get('dw_state') == 1, 'dotwing edit pilot'
                dot_press('start')
                assert get('dw_state') == 2, 'dotwing save pilot'
                dot_press('start')
                for _ in range(200):
                    if get('dw_state') == 3:
                        break
                    run(1)
                assert get('dw_state') == 3, ('dotwing flight', get('dw_state'))
                before = word('dw_frame')
                x = signed('dw_px')
                run(20, ['left'])
                assert signed('dw_px') < x, 'dotwing movement'
                assert word('dw_frame') - before in (19, 20, 21), 'dotwing 60 Hz'
                dot_press('start')
                assert get('dw_state') == 4, 'dotwing pause'
                x = signed('dw_px')
                run(60, ['right'])
                assert signed('dw_px') == x, 'dotwing paused movement'
                dot_press('start')
                assert get('dw_state') == 3, 'dotwing resume'
                if cycle == 0:
                    run(100)
                    shot('dotwing-play')
                run(10, ['start', 'select'])
                run(40)
                settle()
                assert get('arcade_in_game') == 0 and get('launcher_mode') == 1, (slug, 'return')
                assert plays(i) == cycle + 1, (slug, 'plays', plays(i))
                press('right')
                run(20)
                results.append({'game': slug, 'cycle': cycle + 1, 'builder_hangar_play_pause_return': 'pass'})
                continue
            if slug == 'dreambase-invaders':
                # Brand splash, then title, how to play, play, pause and home.
                assert get('db_mode') == 0, ('dreambase splash', get('db_mode'))
                run(90)
                if cycle == 0:
                    shot('dreambase-invaders-splash')
                press('b')
                run(30)
                assert get('db_mode') == 1, ('dreambase title', get('db_mode'))
                run(60)
                if cycle == 0:
                    shot('dreambase-invaders-title')
                press('b')
                run(30)
                assert get('db_mode') == 2, ('dreambase help', get('db_mode'))
                press('start')
                for _ in range(300):
                    if get('db_mode') == 3 and get('pl_phase') == 0:
                        break
                    run(1)
                assert get('db_mode') == 3 and get('pl_paused') == 0, ('dreambase play', get('db_mode'))
                before = get('db_frame')
                x = signed('pl_x')
                run(60, ['left'])
                assert signed('pl_x') < x, 'dreambase movement'
                assert (get('db_frame') - before) & 255 in (59, 60, 61), 'dreambase 60 Hz'
                press('start')
                assert get('pl_paused') == 1, 'dreambase pause'
                x = signed('pl_x')
                run(60, ['right'])
                assert signed('pl_x') == x, 'dreambase paused movement'
                press('start')
                assert get('pl_paused') == 0, 'dreambase resume'
                if cycle == 0:
                    run(200, ['right'])
                    shot('dreambase-invaders-play')
                run(10, ['start', 'select'])
                run(40)
                settle()
                assert get('arcade_in_game') == 0 and get('launcher_mode') == 1, (slug, 'return')
                assert plays(i) == cycle + 1, (slug, 'plays', plays(i))
                press('right')
                run(20)
                results.append({'game': slug, 'cycle': cycle + 1, 'splash_title_help_play_pause_return': 'pass'})
                continue
            assert get(state) == 0, (slug, 'title', get(state))
            if cycle == 0:
                shot(f'{slug}-title')
            press('b')
            assert get(state) == 1, (slug, 'help')
            if slug == 'dot-swarm':
                press('start')
                run(20)
                assert get(state) in (2,4), ('dot swarm play',get(state))
                shot('dot-swarm-play')
                run(60,['start','select']);run(60);settle()
                assert get('arcade_in_game')==0 and get('launcher_mode')==1
                assert plays(i)==cycle+1
                press('right');run(20)
                results.append({'game':slug,'cycle':cycle+1,'launch_help_play_return':'pass'})
                continue
            press('start')
            run(60)
            assert get(state) == 2, (slug, 'play')
            if i != HELLO:
                before_ticks = word('ticks')
                run(120)
                assert word('ticks') - before_ticks in (59,60), (slug, 'timing')
            else:
                run(120)
            press('start')
            assert get(state) == 3, (slug, 'pause')
            run(60)
            assert get(state) == 3
            press('start')
            assert get(state) == 2, (slug, 'resume')
            if cycle == 0 and slug == 'neon-wake':
                # Drive a full round with the brake on to earn a real score.
                for _ in range(80):
                    if get('phase') == 4:
                        break
                    run(60, ['b'])
                assert get('phase') == 4 and word('score') > 0, 'neon round'
                earned = word('best')
            run(20, ['right', 'a'])
            run(20)
            if cycle == 0:
                shot(f'{slug}-play')
            run(10, ['start', 'select'])
            run(40)
            assert get('arcade_in_game') == 0 and get('launcher_mode') == 1, (slug, 'return')
            assert plays(i) == cycle + 1, (slug, 'plays', plays(i))
            press('right')
            run(20)
            results.append({'game': slug, 'cycle': cycle + 1, 'launch_help_play_pause_return': 'pass'})
    assert best(0) == earned > 0, ('neon best', best(0), earned)
    assert get('menu_index') == 0
    press('left')
    run(20)
    assert get('menu_index') == LAST, 'wrap left'
    press('right')
    run(20)
    assert get('menu_index') == 0, 'wrap right'
    press('b')
    assert get('launcher_mode') == 2, 'info'
    shot('launcher-info')
    press('b')
    press('select')
    assert get('launcher_mode') == 3 and get('saves_ok') == 1, 'records'
    shot('launcher-records')
    before = p.screen.ndarray.copy()
    press('right')
    assert (before != p.screen.ndarray).any(), 'records second page'
    shot('launcher-records-page-2')
    press('b')
    saved = io.BytesIO()
finally:
    p.stop(ram_file=saved)

# Power cycle with the saved RAM: records and the last game come back.
pilot_ram = saved.getvalue()[0x200:0x240]
erased_ram = io.BytesIO()
boot(saved.getvalue())
try:
    run(120)
    press('start')
    run(20)
    assert best(0) == earned and plays(0) == 2 and plays(LAST) == 2, 'persisted records'
    assert get('menu_index') == LAST, ('last played game', get('menu_index'))
    press('select')
    run(6, ['a', 'b'])
    run(130, ['a', 'b'])
    run(20)
    assert best(0) == 0 and plays(0) == 0, 'erase'
    press('b')
    assert get('launcher_mode') == 1
finally:
    p.stop(ram_file=erased_ram)
assert erased_ram.getvalue()[0x200:0x240] == pilot_ram, 'erasing launcher records altered Dotwing pilot SRAM'

# Both shipped layouts preserve every score, play count and the unrelated
# SRAM reserved for Dotwing. The shelf opens on the newest game once.
migrated = {}
for version, count in ((2, 10), (3, 11)):
    old_best = [1200, 340, 0, 0, 0, 9876, 0, 0, 0, 450, 6543][:count]
    old_plays = [3, 1, 0, 0, 0, 7, 0, 0, 0, 2, 9][:count]
    boot(historical_save(version, old_best, old_plays, 5))
    migration_ram = io.BytesIO()
    try:
        run(120)
        assert get('records', 4) == 4, 'save version'
        assert [best(i) for i in range(count)] == old_best, 'migrated best scores'
        assert [plays(i) for i in range(count)] == old_plays, 'migrated plays'
        assert best(LAST) == 0 and plays(LAST) == 0, 'new game starts fresh'
        press('start')
        run(20)
        assert get('menu_index') == LAST, ('shelf opens on the new game', get('menu_index'))
        shot(f'launcher-v{version}-new-game')
        press('select')
        shot(f'launcher-v{version}-migrated-records')
        migrated[str(version)] = {'best': [best(i) for i in range(len(GAMES))], 'last': get('menu_index')}
    finally:
        p.stop(ram_file=migration_ram)
    assert migration_ram.getvalue()[0x200:0x220] == bytes(range(32)), 'migration altered Dotwing SRAM'
(BUILD / 'validation.json').write_text(json.dumps({
    'sha256': hashlib.sha256(data).hexdigest(),
    'neon_wake_saved_best': earned,
    'historical_saves_migrated': migrated,
    'tests': results,
}, indent=2) + '\n')
print(f'PASS: launcher, {len(GAMES)} games launched twice, saved best {earned} survived a power cycle, '
      'erase, version 2 and 3 save migration, header and checksums')
