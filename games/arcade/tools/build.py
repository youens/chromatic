"""Build the Chromatic Arcade cartridge from the nine original game sources.

Each shared-runtime game is compiled from its own unmodified source. Its
file-scope symbols are discovered from the compiler's own output and given a
g<N>_ prefix so eight games can link into one ROM. Code and graphics for each
game get their own 16 KiB banks; the runtime, dispatch, Stormkite's scanline
interrupt handlers and the launcher live in the fixed bank.
"""
from pathlib import Path
import hashlib
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
assert ART_BANK < ROM_BANKS
assert f'#pragma bank {ART_BANK}' in (Path(launcher_art.__file__).read_text())


def replace(source, old, new):
    assert old in source, old
    return source.replace(old, new)


def emit(name, text):
    path = OUT / name
    path.write_text(text)
    return path


def compile_c(source, *includes, asm=False):
    flags = ['-I' + str(i) for i in includes]
    target = OUT / (source.stem + ('.asm' if asm else '.o'))
    subprocess.run([str(LCC), *flags, '-S' if asm else '-c', '-o', str(target), str(source)], check=True)
    return target


def defined_symbols(slug, source_text):
    """Global names a translation unit defines, read from SDCC's assembly."""
    # Compiled from its own directory so "runtime.h" resolves to the original.
    (OUT / 'probe').mkdir(exist_ok=True)
    probe = emit(f'probe/{slug}.c', source_text)
    asm = compile_c(probe, SHARED, ROOT / 'games' / slug / 'build', OUT, asm=True).read_text()
    exported = set(re.findall(r'^\s*\.globl\s+_(\w+)', asm, re.M))
    return exported & set(re.findall(r'^_(\w+)::?', asm, re.M))


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
    names = defined_symbols(slug, source)
    assert {'game_reset', 'game_update', 'game_draw', *metadata} <= names, (slug, names)
    for name in sorted(names, key=len, reverse=True):
        source = re.sub(r'\b' + name + r'\b', prefix + name, source)
    for fn in ['game_reset', 'game_update', 'game_draw']:
        source = replace(source, f'void {prefix}{fn}(void)', f'void {prefix}{fn}(void) BANKED')
        prototypes.append(f'void {prefix}{fn}(void) BANKED;')
    if slug == 'neon-wake':
        source = replace(source, 'extern const uint8_t road_maps[], road_colors[];', 'void g0_road(void) BANKED;')
        source = re.sub(r'  const uint8_t \*r =.*?road_colors.*?;', '', source, flags=re.S)
        start = source.index('  memcpy(tiles + 224, r, 320);')
        end = source.index('  for (i = 0; i < 6; i++) {', start)
        source = source[:start] + '  g0_road();\n' + source[end:]
    source = f'#pragma bank {i * 2 + 1}\n' + source
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
    assets = f'#pragma bank {i * 2 + 2}\n#include "runtime.h"\n#include <string.h>\n' + assets
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
menu = OUT / 'menu.o'
subprocess.run([str(LCC), '-I' + str(OUT), '-I' + str(SHARED), '-c', '-o', str(menu), str(ROOT / 'games/arcade/src/menu.c')], check=True)
rom = OUT / 'chromatic-arcade.gbc'
# MBC5 with battery-backed RAM (0x1B): 32 banks of ROM and one 8 KiB RAM bank.
subprocess.run([str(LCC), '-Wm-yC', '-Wm-yt0x1B', f'-Wm-yo{ROM_BANKS}', '-Wm-ya1', '-Wm-ynCHROMATIC ARCADE',
                '-Wl-m', '-Wl-j', '-o', str(rom), str(menu), *[str(x) for x in objects]], check=True)
# All game RAM must end below the screen buffers the runtime pins at 0xD000.
layout = (OUT / 'chromatic-arcade.map').read_text()
heap = int(re.search(r'([0-9A-F]{8})\s+s__HEAP\b', layout).group(1), 16)
assert heap <= 0xD000, f'WRAM overlaps screen buffers: heap at {heap:#x}'
print(f'{rom}: {rom.stat().st_size} bytes, WRAM free {0xD000 - heap} bytes, SHA-256 {hashlib.sha256(rom.read_bytes()).hexdigest()}')
