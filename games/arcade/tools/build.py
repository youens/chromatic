"""Build the Chromatic Arcade cartridge from the original game sources.

Each shared-runtime game is compiled from its own unmodified source. Its
file-scope symbols are discovered from the compiler's own output and given a
g<N>_ prefix so the games can link into one ROM. Their code and graphics
modules are packed into as few 16 KiB banks as fit; the runtime, dispatch,
Stormkite's scanline interrupt handlers and the launcher's own interrupt
handlers live in the fixed bank, and the rest of the launcher in bank 31.
Large per-game arrays are moved out of low work RAM, which every game keeps
for good, into a slot of upper work RAM that only the running game uses.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'games/arcade/build'
OUT.mkdir(exist_ok=True)
LCC = ROOT / '.tools/gbdk/bin/lcc'
SHARED = ROOT / 'games/shared'
sys.path.insert(0, str(Path(__file__).resolve().parent))
import launcher_art  # noqa: E402

# Runtime games in dispatch order. Extra sources stay in the fixed bank
# (interrupt handlers must never be banked out).
GAMES = [
    ('neon-wake', []), ('moonthread', []), ('echo-vault', []),
    ('bloom-circuit', []), ('orbit-choir', []), ('stormkite', ['src/sky.c']),
    ('comet-links', []), ('prism-well', []), ('dot-swarm', []),
]
CLEANUP = {'stormkite': 'sky_stop();'}
HELLO_BANK = 1 + 2 * len(GAMES)
ART_BANK = HELLO_BANK + 2
ROM_BANKS = 32
LAUNCHER_BANK = ROM_BANKS - 1
assert ART_BANK < ROM_BANKS
# Runtime games' modules are packed by GBDK's bankpack into banks below the
# native games; whatever they leave free is room for future games.
PACKED_BANKS = (1, HELLO_BANK - 1)
assert f'#pragma bank {ART_BANK}' in (Path(launcher_art.__file__).read_text())


def replace(source, old, new):
    assert old in source, old
    return source.replace(old, new)


def emit(name, text):
    path = OUT / name
    path.write_text(text)
    return path


def compile_c(source, *includes, asm=False):
    flags = ['-Wf--opt-code-size', *['-I' + str(i) for i in includes]]
    target = OUT / (source.stem + ('.asm' if asm else '.o'))
    subprocess.run([str(LCC), *flags, '-S' if asm else '-c', '-o', str(target), str(source)], check=True)
    return target


def defined_symbols(slug, source_text):
    """Global names a translation unit defines, and the sizes of its
    uninitialized variables, read from SDCC's assembly."""
    # Compiled from its own directory so "runtime.h" resolves to the original.
    (OUT / 'probe').mkdir(exist_ok=True)
    probe = emit(f'probe/{slug}.c', source_text)
    asm = compile_c(probe, SHARED, ROOT / 'games' / slug / 'build', OUT, asm=True).read_text()
    exported = set(re.findall(r'^\s*\.globl\s+_(\w+)', asm, re.M))
    sizes = {name: int(size) for name, size in re.findall(r'^_(\w+)::\s*\n\s*\.ds\s+(\d+)', asm, re.M)}
    return exported & set(re.findall(r'^_(\w+)::?', asm, re.M)), sizes


# Shared-runtime games keep their screen buffers at D000-D7FF and the stack
# keeps DC00-DFFF, so D800-DBFF is free while a runtime game runs. Each
# array listed here moves into that slot. All of them are rebuilt by the
# game's game_reset, or written before they are read, so nothing depends on
# a previous launch; Dreambase Invaders (D000-D7DF) and Dotwing (D000-DBFF)
# reuse the same memory only while they run.
OVERLAY_BASE, OVERLAY_END = 0xD800, 0xDC00
OVERLAYS = {
    'echo-vault': ['maze', 'seen', 'stack', 'distmap'],
    'bloom-circuit': ['board', 'solution', 'powered', 'visit', 'path', 'undo_cell', 'undo_value'],
    'prism-well': ['board', 'marks'],
    'dot-swarm': ['snakes', 'fx', 'fy'],
}


def split_declarators(text):
    parts, depth, current = [], 0, ''
    for ch in text:
        depth += ch == '['
        depth -= ch == ']'
        if ch == ',' and not depth:
            parts.append(current.strip())
            current = ''
        else:
            current += ch
    return parts + [current.strip()]


def overlay(source, prefix, names, sizes):
    """Give each named file-scope array its own declaration at a fixed
    address in the overlay slot."""
    address, placed = OVERLAY_BASE, {}
    for name in names:
        full = prefix + name
        assert name in sizes, (name, 'must be an uninitialized variable')
        statement = re.search(r'^([A-Za-z_]\w*)[ \t]+([^;{}()=]*\b' + full + r'\s*\[[^;{}()=]*);', source, re.M)
        assert statement, full
        kind, declarators = statement.group(1), split_declarators(statement.group(2))
        target = next(d for d in declarators if re.match(full + r'\s*\[', d))
        rest = [d for d in declarators if d is not target]
        text = (f'{kind} {", ".join(rest)};\n' if rest else '') + f'{kind} __at({address:#06x}) {target};'
        source = source[:statement.start()] + text + source[statement.end():]
        placed[full] = [address, sizes[name]]
        address += sizes[name]
    assert address <= OVERLAY_END, (prefix, f'overlay ends at {address:#x}')
    return source, placed


header = (SHARED / 'runtime.h').read_text()
header = replace(header, 'extern const char game_title[], game_tagline[], game_controls1[],\n    game_controls2[], game_goal[];', 'extern char game_title[21], game_tagline[21], game_controls1[21], game_controls2[21], game_goal[21];')
header = replace(header, 'extern const uint16_t game_palette[32];', 'extern uint16_t game_palette[32];')
header += '\nextern uint8_t arcade_selected;\nvoid arcade_load_assets(void);\nvoid arcade_title_tiles(void);\nvoid arcade_backdrop(void);\nvoid arcade_run(void);\nvoid arcade_cleanup(void);\n'
emit('runtime.h', header)
for name in ('digits.h',):
    emit(name, (SHARED / name).read_text())

metadata = ['game_title', 'game_tagline', 'game_controls1', 'game_controls2', 'game_goal', 'game_palette']
objects = []
prototypes = []
unbanked = []
overlays = {}
for i, (slug, extras) in enumerate(GAMES):
    prefix = f'g{i}_'
    game = ROOT / 'games' / slug
    source = (game / 'src/main.c').read_text()
    # Inline generated course or level headers so their tables are renamed too.
    for include in re.findall(r'#include "([\w-]+\.h)"', source):
        local = game / 'build' / include
        if local.exists() and include != 'assets.h':
            source = replace(source, f'#include "{include}"', local.read_text().replace('#include <stdint.h>\n', ''))
    for extra in extras:
        emit(Path(extra).name.replace('.c', '.h'), (game / extra).with_suffix('.h').read_text())
    names, sizes = defined_symbols(slug, source)
    assert {'game_reset', 'game_update', 'game_draw', *metadata} <= names, (slug, names)
    for name in sorted(names, key=len, reverse=True):
        source = re.sub(r'\b' + name + r'\b', prefix + name, source)
    if slug in OVERLAYS:
        source, overlays[slug] = overlay(source, prefix, OVERLAYS[slug], sizes)
    for fn in ['game_reset', 'game_update', 'game_draw']:
        source = replace(source, f'void {prefix}{fn}(void)', f'void {prefix}{fn}(void) BANKED')
        prototypes.append(f'void {prefix}{fn}(void) BANKED;')
    if slug == 'neon-wake':
        source = replace(source, 'extern const uint8_t road_maps[], road_colors[];', 'void g0_road(void) BANKED;')
        source = re.sub(r'  const uint8_t \*r =.*?road_colors.*?;', '', source, flags=re.S)
        start = source.index('  memcpy(tiles + 224, r, 320);')
        end = source.index('  for (i = 0; i < 6; i++) {', start)
        source = source[:start] + '  g0_road();\n' + source[end:]
    source = '#pragma bank 255\n' + source
    source += f'\nvoid {prefix}metadata(void) BANKED {{\n'
    source += '  memcpy(game_palette, ' + prefix + 'game_palette, sizeof(game_palette));\n'
    for name in metadata[:-1]:
        source += f'  strcpy({name}, {prefix}{name});\n'
    source += '}\n'
    source = '#include <string.h>\n' + source
    emit(f'g{i}.c', source)
    prototypes.append(f'void {prefix}metadata(void) BANKED;')
    assets = (game / 'build/assets.h').read_text()
    assets = re.sub(r'^const uint8_t', 'static const uint8_t', assets, flags=re.M)
    assets = '#pragma bank 255\n#include "runtime.h"\n#include <string.h>\n' + assets
    assets += f'\nvoid {prefix}assets(void) BANKED {{\nset_bkg_data(0,192,bg_data);\nset_bkg_data(192,TITLE_COUNT,title_data);\nset_sprite_data(0,SPRITE_COUNT,sprite_data);\n}}\n'
    assets += f'void {prefix}backdrop(void) BANKED {{ memcpy(tiles,scene_map,1024); memcpy(colors,scene_colors,1024); }}\n'
    assets += f'void {prefix}title(void) BANKED {{ uint8_t x,y; for(y=0;y<6;y++) for(x=0;x<20;x++) {{ tile(x,y+3,title_map[y*20+x],arcade_selected==8 && y>=3 ? 2 : 1); }} }}\n'
    if i == 0:
        assets += '''extern int8_t g0_curve, g0_previous_curve;
extern uint16_t g0_distance_run;
void g0_road(void) BANKED {
memcpy(tiles+224,road_maps+(uint16_t)(g0_curve+4)*640+((g0_distance_run>>2)&1)*320,320);
if(g0_previous_curve!=g0_curve) {
memcpy(colors+224,road_colors+(uint16_t)(g0_curve+4)*320,320);
g0_previous_curve=g0_curve;
}
}
'''
    emit(f'a{i}.c', assets)
    for fn in ['assets', 'backdrop', 'title']:
        prototypes.append(f'void {prefix}{fn}(void) BANKED;')
    for extra in extras:
        unbanked.append(emit(f'{slug}-{Path(extra).name}', (game / extra).read_text()))
        prototypes.append((game / extra).with_suffix('.h').read_text().split('#include <stdint.h>\n', 1)[-1].replace('#endif', ''))

runtime = (SHARED / 'runtime.c').read_text()
runtime = replace(runtime, '#include "assets.h"', '')
runtime = replace(runtime, '  memcpy(tiles, scene_map, 1024);\n  memcpy(colors, scene_colors, 1024);', '  arcade_backdrop();')
runtime = replace(runtime, '  for (y = 0; y < 6; y++)\n    for (x = 0; x < 20; x++)\n      tile(x, y + 3, title_map[y * 20 + x], 1);', '  arcade_title_tiles();')
runtime = replace(runtime, 'void main(void)', 'void arcade_run(void)')
# The launcher restores each game's saved best score before it starts.
runtime = replace(runtime, '  cpu_fast();', '  phase=previous=muted=half=clock_ticks=0;\n  title_ready=1;')
runtime = replace(runtime, '  set_bkg_data(0, 192, bg_data);\n  set_bkg_data(192, TITLE_COUNT, title_data);\n  set_sprite_data(0, SPRITE_COUNT, sprite_data);', '  arcade_load_assets();')
runtime = replace(runtime, '    current = joypad();', '    current = joypad();\n    if ((current & (J_START|J_SELECT)) == (J_START|J_SELECT)) return;')
dot_runtime = (ROOT / 'games/dot-swarm/src/runtime.c').read_text()
dot_title = dot_runtime[dot_runtime.index('static void title(void)'):dot_runtime.index('static void help(void)')]
dot_title = dot_title.replace('static void title(void)', 'static void dot_title(void)')
dot_title = dot_title.replace('  for (y = 0; y < 6; y++)\n    for (x = 0; x < 20; x++)\n      tile(x, y + 3, title_map[y * 20 + x], y < 3 ? 1 : 2);', '  arcade_title_tiles();')
runtime = runtime.replace('static void title(void)', dot_title+'\nstatic void title(void)')
runtime = runtime.replace('static void title(void) {', 'static void title(void) {\n  if(arcade_selected==8) { dot_title(); return; }')
emit('runtime.c', runtime)

hello = (ROOT / 'games/hello-dot/src/main.c').read_text()
hello = replace(hello, '#include "assets.h"', '#include "hello-assets.h"')
hello = replace(hello, 'void main(void)', 'void hello_run(void) BANKED')
hello = replace(hello, '    cpu_fast();', '    menu_frame=scene=muted=last_keys=banner_time=0;')
hello = replace(hello, '        keys=joypad();', '        keys=joypad();\n        if ((keys & (J_START|J_SELECT)) == (J_START|J_SELECT)) return;\n        ')
hello = re.sub(r'\bmuted\b', 'hello_muted', hello)
emit('hello.c', f'#pragma bank {HELLO_BANK}\n' + hello)
emit('hello-assets.h', (ROOT / 'games/hello-dot/build/assets.h').read_text())
hgame = (ROOT / 'games/hello-dot/src/game.c').read_text()
hheader = (ROOT / 'games/hello-dot/src/game.h').read_text()
for fn in ['game_init', 'game_tick', 'game_seconds']:
    hgame = re.sub(r'(\b' + fn + r'\([^)]*\))', r'\1 BANKED', hgame, count=1)
    hheader = re.sub(r'(\b' + fn + r'\([^)]*\))', r'\1 BANKED', hheader, count=1)
emit('game.h', '#include <gb/gb.h>\n' + hheader)
emit('hello-game.c', f'#pragma bank {HELLO_BANK + 1}\n' + hgame)
prototypes.append('void hello_run(void) BANKED;\nextern uint16_t best_score;')

# Dreambase Invaders also keeps its own engine. fixed.c holds its interrupt
# handlers and the helpers that take pointers into a caller's bank, so it
# stays in the fixed bank; each other module gets a bank of its own.
DREAMBASE = ROOT / 'games/dreambase-invaders'
DB_BANKS = {'main': ART_BANK + 1, 'screens': ART_BANK + 2, 'play': ART_BANK + 3, 'art': ART_BANK + 4}
assert max(DB_BANKS.values()) < ROM_BANKS
db_sources = [emit(f'dbi-{name}.c', f'#pragma bank {bank}\n' + (DREAMBASE / 'src' / f'{name}.c').read_text())
              for name, bank in DB_BANKS.items()]
db_sources.append(emit('dbi-fixed.c', (DREAMBASE / 'src/fixed.c').read_text()))
prototypes.append('void dreambase_run(void) BANKED;\nextern uint16_t db_best;')
# Dotwing has its own 60 Hz flight engine. Its modules share banks exactly
# as in the standalone cartridge, so every banked call sees the same data;
# only vbl.c, the sound driver's VBlank hook, lives in the fixed bank.
DOTWING = ROOT / 'games/dotwing'
DW_BANKS = {'main': ART_BANK + 5, 'terraina': ART_BANK + 5, 'screens': ART_BANK + 6,
            'terrainb': ART_BANK + 6, 'play': ART_BANK + 7, 'art': ART_BANK + 8,
            'fixed': ART_BANK + 9, 'rivals': ART_BANK + 9, 'world': ART_BANK + 10}
assert max(DW_BANKS.values()) < ROM_BANKS
dw_sources = [emit(f'dw-{name}.c', f'#pragma bank {bank}\n' + (DOTWING / 'src' / f'{name}.c').read_text())
              for name, bank in DW_BANKS.items()]
dw_sources.append(emit('dw-vbl.c', (DOTWING / 'src/vbl.c').read_text()))
prototypes.append('void dotwing_run(void) BANKED;\nextern uint16_t dw_best;')
prototypes.append('void launcher_art(void) BANKED;\nvoid arcade_metadata(void);')
emit('modules.h', '\n'.join(prototypes) + '\n')

dispatch = '#include "runtime.h"\n#include "modules.h"\n'
dispatch += 'char game_title[21], game_tagline[21], game_controls1[21], game_controls2[21], game_goal[21];\nuint16_t game_palette[32];\n'
for common, module in [('game_reset', 'game_reset'), ('game_update', 'game_update'), ('game_draw', 'game_draw'),
                       ('arcade_metadata', 'metadata'), ('arcade_load_assets', 'assets'),
                       ('arcade_title_tiles', 'title'), ('arcade_backdrop', 'backdrop')]:
    dispatch += f'void {common}(void) {{ switch(arcade_selected) {{\n'
    for i in range(len(GAMES)):
        dispatch += f'case {i}: g{i}_{module}(); break;\n'
    dispatch += '} }\n'
dispatch += '/* Undo any hardware a game claimed beyond the shared runtime. */\n'
dispatch += 'void arcade_cleanup(void) { ' + ' '.join(CLEANUP.values()) + ' }\n'
emit('dispatch.c', dispatch)

launcher_art.build()
sources = ['runtime', 'dispatch', *[part + str(i) for i in range(len(GAMES)) for part in ['g', 'a']], 'hello', 'hello-game', 'launcher-art']
for name in sources:
    objects.append(compile_c(OUT / f'{name}.c', OUT, SHARED))
for path in unbanked:
    objects.append(compile_c(path, OUT, SHARED))
for game, native_sources in ((DREAMBASE, db_sources), (DOTWING, dw_sources)):
    for path in native_sources:
        target = OUT / (path.stem + '.o')
        subprocess.run([str(LCC), '-DARCADE', '-Wf--opt-code-size', '-I' + str(game / 'src'),
                        '-I' + str(game / 'build'), '-c', '-o', str(target), str(path)], check=True)
        objects.append(target)
# The launcher's drawing, menus and save records run from a switchable bank;
# its data is compiled into the same bank, and every call it makes passes
# only pointers into that bank or work RAM. shelf.c keeps the interrupt
# handlers and the entry point in the fixed bank.
menu = compile_c(emit('menu.c', f'#pragma bank {LAUNCHER_BANK}\n' + (ROOT / 'games/arcade/src/menu.c').read_text()), OUT, SHARED)
shelf = compile_c(ROOT / 'games/arcade/src/shelf.c', OUT, SHARED)
rom = OUT / 'chromatic-arcade.gbc'
# MBC5 with battery-backed RAM (0x1B): 32 banks of ROM and one 8 KiB RAM bank.
subprocess.run([str(LCC), '-autobank', f'-Wb-min={PACKED_BANKS[0]}', f'-Wb-max={PACKED_BANKS[1]}',
                '-Wm-yC', '-Wm-yt0x1B', f'-Wm-yo{ROM_BANKS}', '-Wm-ya1', '-Wm-ynCHROMATIC ARCADE',
                '-Wl-m', '-Wl-j', '-o', str(rom), str(shelf), str(menu), *[str(x) for x in objects]], check=True)
layout = (OUT / 'chromatic-arcade.map').read_text()


def bank_usage(layout):
    """Bytes placed in each ROM bank, from the linker map. The linker only
    warns when areas overlap, so check every bank's areas here."""
    areas, fixed = [], 0
    for name, addr, size, kind in re.findall(r'^(_\w+)\s+([0-9A-F]{8})\s+([0-9A-F]{8}) =.*\((ABS|REL)', layout, re.M):
        addr, size = int(addr, 16), int(size, 16)
        if kind == 'ABS':
            fixed += size      # vectors and the cartridge header, below 0x200
        elif size and (addr & 0xFFFF) < 0x8000:
            areas.append((addr >> 16, addr & 0xFFFF, size, name))
    used = {0: fixed}
    for bank in sorted({a[0] for a in areas}):
        spans = sorted((start, start + size, name) for b, start, size, name in areas if b == bank)
        low, high = (0x0000, 0x4000) if bank == 0 else (0x4000, 0x8000)
        for (start, end, name), following in zip(spans, spans[1:] + [(high, high, '')]):
            assert low <= start and end <= high, f'{name} overflows bank {bank}: {start:#x}-{end:#x}'
            assert end <= following[0], f'{name} overlaps {following[2]} in bank {bank}'
        used[bank] = used.get(bank, 0) + sum(end - start for start, end, name in spans)
    return used


used = bank_usage(layout)
assert max(used) < ROM_BANKS and rom.stat().st_size == ROM_BANKS * 16384
# All ordinary game RAM must end below the screen buffers the runtime pins
# at 0xD000; the overlay slot above them ends where the stack's room begins.
heap = int(re.search(r'([0-9A-F]{8})\s+s__HEAP\b', layout).group(1), 16)
assert heap <= 0xD000, f'WRAM overlaps screen buffers: heap at {heap:#x}'
emit('overlays.json', json.dumps(overlays, indent=2) + '\n')
packed = sorted(b for b in used if PACKED_BANKS[0] <= b <= PACKED_BANKS[1])
print(f'{rom}: {rom.stat().st_size} bytes, WRAM free {0xD000 - heap} bytes, '
      f'{PACKED_BANKS[1] - PACKED_BANKS[0] + 1 - len(packed)} empty banks, '
      f'overlays {", ".join(f"{slug} {sum(s for a, s in v.values())}" for slug, v in overlays.items())} bytes, '
      f'SHA-256 {hashlib.sha256(rom.read_bytes()).hexdigest()}')
