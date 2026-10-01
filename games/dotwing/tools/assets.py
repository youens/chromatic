#!/usr/bin/env python3
"""Build every Dotwing graphic from exact native pixels.

Writes src/art_ids.h (tile and sprite numbers), src/gfx_data.h (shared tiles,
sprites and palettes, read by src/art.c), src/terraina_data.h and
src/terrainb_data.h (sector scenery and level layouts, read by
src/terraina.c and src/terrainb.c) and src/boss_data.h (background-layer
bosses, read by src/world.c), plus native previews in art/. Nothing is
downsampled from generated imagery: every pixel is placed by the code in
this directory.
"""
from pathlib import Path
import sys

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gbgfx import Pic, encode_tile, sprite_tiles, BgSheet, c_array, cgb, hexrgb  # noqa: E402,F401
import font  # noqa: E402
import ui  # noqa: E402
import sprites as S  # noqa: E402
import enemies as E  # noqa: E402
import sectors as SEC  # noqa: E402
import bosses as B  # noqa: E402
import menuart as M  # noqa: E402
from palette import *  # noqa: E402,F403

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'src'
ART = ROOT / 'art'
ids = []


def define(name, value):
    ids.append(f'#define {name} {value}')


def pals(name, palettes):
    flat = [cgb(c) for p in palettes for c in p]
    return f'static const palette_color_t {name}[] = {{' + ','.join(hex(v) for v in flat) + '};\n'


# ------------------------------------------------- background, VRAM bank 0
bg = []
for name, cell in ui.hud_tiles():
    define('DW_T_' + name, len(bg))
    bg.append(cell)
assert len(bg) <= 32, len(bg)
while len(bg) < 32:
    bg.append(ui.blank())
bg += font.font_tiles()
assert len(bg) == 96
for name, cell in ui.panel_tiles():
    define('DW_T_PANEL_' + name, len(bg))
    bg.append(cell)
for name, cell in ui.boss_bar_tiles():
    define('DW_T_' + name, len(bg))
    bg.append(cell)
icons = M.bg_icons()
for name, cells in icons:
    define('DW_T_' + name, len(bg))
    bg += cells
assert len(bg) <= 128, len(bg)
define('DW_BG_COMMON', len(bg))

# ---------------------------------------------- sprites, OBJ VRAM bank 0
obj = []


def add_sprite(name, pic, frames=1):
    define('DW_S_' + name, len(obj) // 16)
    obj.extend(sprite_tiles(S.pad16(pic) if pic.h % 16 or pic.w % 8 else pic, [1, 2, 3]))


add_sprite('PLANE', S.PLANE)
add_sprite('PLANE_BANK', S.PLANE_BANK)
for i, f in enumerate(S.FLAME):
    add_sprite('FLAME' if i == 0 else 'FLAME%d' % i, f)
add_sprite('SHADOW', S.SHADOW)
add_sprite('SHOT', S.SHOT)
add_sprite('SHOT_L', S.SHOT_L)
add_sprite('SHOT_R', S.SHOT_R)
add_sprite('SHOT_SOLO', S.SHOT_SOLO)
add_sprite('ALLY', S.ALLY[0])
add_sprite('ALLY1', S.ALLY[1])
add_sprite('CORE', S.CORE)
add_sprite('BULLET', S.BULLET_SMALL)
add_sprite('BULLET_BIG', S.BULLET_BIG)
add_sprite('BULLET_NEEDLE', S.BULLET_NEEDLE)
add_sprite('BULLET_STAR', S.BULLET_STAR)
for i, c in enumerate(S.COIN):
    add_sprite('COIN' if i == 0 else 'COIN%d' % i, c)
add_sprite('POWER', S.POWER)
add_sprite('REPAIR', S.REPAIR)
add_sprite('BOLT', S.BOLT)
add_sprite('ALLY_PICK', S.ALLY[0])
for i, e in enumerate(S.EXPLOSION):
    add_sprite('BOOM' if i == 0 else 'BOOM%d' % i, e)
add_sprite('SPARK', S.SPARK[0])
add_sprite('SPARK1', S.SPARK[1])
add_sprite('RING', S.RING)
add_sprite('SPEED', S.SPEEDLINE)
define('DW_S_FLEET', len(obj) // 16)
fleet_base = len(obj) // 16
assert fleet_base <= 96, fleet_base
FLEET_TILES = 32
fleet_data = []
for lab in range(4):
    d, g, h = E.fleet(lab)
    data = []
    for pic in d + g + h:
        data += sprite_tiles(pic, [1, 2, 3])
    assert len(data) == FLEET_TILES * 16, len(data)
    fleet_data.append(data)
define('DW_S_DRONE', fleet_base)
define('DW_S_GUNNER', fleet_base + 8)
define('DW_S_HEAVY', fleet_base + 16)
core_base = fleet_base + FLEET_TILES
define('DW_S_BOSSCORE', core_base)
core_data = []
for f in range(2):
    core_data += sprite_tiles(M.boss_core(f), [1, 2, 3])
assert core_base + len(core_data) // 16 <= 128

# ----------------------------------------------- menu sprites, OBJ bank 1
menu_obj = []


def add_menu(name, pic):
    define('DW_M_' + name, len(menu_obj) // 16)
    menu_obj.extend(sprite_tiles(pic, [1, 2, 3]))


add_menu('HERO', M.hero_plane())
add_menu('HERO_GLASS', M.hero_glass())
add_menu('CLOUD', M.title_cloud(0))
add_menu('CLOUD_SMALL', M.title_cloud(1))
add_menu('CURSOR', M.cursor())
add_menu('SPARKLE', M.sparkle())
for k in (1, 2, 3):
    add_menu('SWATCH%d' % k, M.swatch(k))
add_menu('RING', M.ring_cursor())
define('DW_M_PORTRAIT', len(menu_obj) // 16)
portrait_tiles = len(menu_obj) // 16
menu_obj.extend([0] * (16 * 16))     # body, loaded per shape
define('DW_M_FACE', len(menu_obj) // 16)
menu_obj.extend([0] * (4 * 16))      # face, loaded per face
define('DW_M_GEAR', len(menu_obj) // 16)
menu_obj.extend([0] * (8 * 16))      # gear, loaded per gear
assert len(menu_obj) // 16 <= 128, len(menu_obj) // 16
bodies = sum((sprite_tiles(M.portrait_body(s), [1, 2, 3]) for s in range(5)), [])
faces = sum((sprite_tiles(M.portrait_face(f), [1, 2, 3]) for f in range(4)), [])
gears = sum((sprite_tiles(M.portrait_gear(g), [1, 2, 3]) for g in range(4)), [])
gear_y = [M.GEAR_Y[g] for g in range(4)]

# ------------------------------------------------------------- palettes
livery = [[INK, INK, c, M.tint(c)] for c in LIVERY]
lab_obj = [list(LABS[i][2]) + list(LABS[i][0]) + list(LABS[i][1]) for i in range(4)]
hud = [[M.HUD_PANEL, M.HUD_EDGE, GOLD, WHITE], [M.HUD_PANEL, M.HUD_EDGE, '#ff5a8a', CYAN]]

gfx = '/* Generated by tools/assets.py; edit the Python art, not this file. */\n'
gfx += c_array('bg_common', sum((encode_tile(t) for t in bg), []))
gfx += c_array('obj_flight', obj)
gfx += c_array('obj_fleets', sum(fleet_data, []))
gfx += c_array('obj_boss_core', core_data)
gfx += c_array('obj_menu', menu_obj)
gfx += c_array('obj_bodies', bodies)
gfx += c_array('obj_faces', faces)
gfx += c_array('obj_gears', gears)
gfx += c_array('gear_y', gear_y)
gfx += pals('pal_obj_flight', OBJ_FLIGHT)
gfx += pals('pal_livery', livery)
gfx += pals('pal_labs', [p for i in range(4) for p in (LABS[i][2], LABS[i][0], LABS[i][1])])
gfx += pals('pal_hud', hud)
gfx += pals('pal_menu_bg', M.MENU_BG)
gfx += pals('pal_menu_obj', M.MENU_OBJ)
title = BgSheet(M.TITLE_BG, limit=256)
title_rows = title.map(M.title_scene())
gfx += c_array('title_tiles', title.data())
title_map = []
for row in title_rows:
    for num, flip, pal in row:
        title_map += [128 + (num & 127), pal | flip | (0x08 if num >= 128 else 0)]
gfx += c_array('title_map', title_map)
gfx += pals('pal_title', M.TITLE_BG)
define('DW_TITLE_TILES', len(title.tiles))
define('DW_TITLE_TEXT_ROW', M.TITLE_TEXT_ROW)
define('DW_TITLE_INFO_ROW', M.TITLE_INFO_ROW)
(SRC / 'gfx_data.h').write_text(gfx)

# ---------------------------------------------------------------- world
# Scenery for sectors 1-2 and 3-4 lives in two banks. A loader in each bank
# uploads tiles and copies the layout tables into work RAM at 0xD680.
LIMITS = {'base': 64, 'stamps': 64, 'place': 480, 'cells': 800}
terrain = ['/* Generated by tools/assets.py; edit the Python art, not this file. */\n' for _ in range(2)]
sector_info = []
for i, make in enumerate(SEC.SECTORS):
    s = make().build()
    n = s.tiles_used()
    tiles = []
    for t in s.sheet.tiles:
        tiles += encode_tile([list(t[y * 8:y * 8 + 8]) for y in range(8)])
    cells, index = [], []
    for sid, (name, w, h, rows) in enumerate(s.stamp_cells):
        index += [w, h, len(cells) // 2 & 255, len(cells) // 2 >> 8]
        for y in range(h):
            for x in range(w):
                num, attr = s.cell(sid, x, y)
                cells += [128 + (num & 127), attr]
    place = []
    for row, col, sid, flip in s.placements:
        place += [row & 255, row >> 8, col, sid | (0x80 if flip else 0)]
    place += [255, 255, 0, 0]
    base = []
    for t in SEC.BASE_PICK:
        num, attr = s.base[t]
        base += [128 + (num & 127), attr | (0x08 if num >= 128 else 0)]
    for name, data in (('base', base), ('stamps', index), ('place', place), ('cells', cells)):
        assert len(data) <= LIMITS[name], (i, name, len(data))
    k = i // 2
    terrain[k] += c_array(f's{i}_tiles', tiles)
    terrain[k] += c_array(f's{i}_stamps', index) + c_array(f's{i}_cells', cells)
    terrain[k] += c_array(f's{i}_place', place) + c_array(f's{i}_base', base)
    terrain[k] += pals(f's{i}_pal', s.palettes)
    terrain[k] += f'#define S{i}_NTILES {n}\n'
    sector_info.append((n, len(s.placements), len(s.sheet.quantised)))
    s.preview(1).save(ART / f'level-{i + 1}.png')
for k in range(2):
    (SRC / f'terrain{"ab"[k]}_data.h').write_text(terrain[k])
world = '/* Generated by tools/assets.py; edit the Python art, not this file. */\n'
boss_info = []
boss_images = []


def render_cells(rows, tiles, palettes):
    """A background map as it appears on screen."""
    im = Image.new('RGB', (len(rows[0]) * 8, len(rows) * 8))
    for r, row in enumerate(rows):
        for c, (num, flip, pal) in enumerate(row):
            colours, t = palettes[pal], tiles[num]
            for y in range(8):
                for x in range(8):
                    sx = 7 - x if flip & 0x20 else x
                    sy = 7 - y if flip & 0x40 else y
                    im.putpixel((c * 8 + x, r * 8 + y), hexrgb(colours[t[sy * 8 + sx]]))
    return im


for i, make in enumerate(B.BOSSES):
    pic, palettes, hit, base = make()
    assert base == SEC.SECTORS[i % len(SEC.SECTORS)]().palettes[0][0] or len(SEC.SECTORS) < 4, ('boss arena colour', i)
    sheet = BgSheet([[base] * 4] + palettes, limit=128)
    rows = sheet.map(pic, fill=base)
    world += c_array(f'b{i}_tiles', sheet.data())
    cells = []
    for row in rows:
        for num, flip, pal in row:
            cells += [num, pal | flip | 0x08]
    world += c_array(f'b{i}_map', cells)
    world += pals(f'b{i}_pal', palettes)
    boss_info.append((len(sheet.tiles), len(sheet.quantised)))
    boss_images.append(render_cells(rows, sheet.tiles, [[base] * 4] + palettes))
for name in ('tiles', 'map'):
    world += f'static const uint8_t * const boss_{name}[4] = {{' + ','.join(f'b{i}_{name}' for i in range(4)) + '};\n'
world += 'static const palette_color_t * const boss_pal[4] = {' + ','.join(f'b{i}_pal' for i in range(4)) + '};\n'
world += 'static const uint8_t boss_ntiles[4] = {' + ','.join(str(n) for n, q in boss_info) + '};\n'
define('DW_BOSS_W', B.W // 8)
define('DW_BOSS_H', B.H // 8)
define('DW_LEVEL_ROWS', SEC.ROWS)
define('DW_LEVEL_WIDTH', SEC.WIDTH)
(SRC / 'boss_data.h').write_text(world)

header = '/* Generated by tools/assets.py. Tile and sprite numbers. */\n#ifndef DOTWING_ART_IDS_H\n#define DOTWING_ART_IDS_H\n'
header += '\n'.join(ids) + '\n#endif\n'
(SRC / 'art_ids.h').write_text(header)
M.previews(ART)


# ------------------------------------------------------- native previews
def sprite_image(pic, palette):
    """A sprite in one object palette; colour 0 is transparent."""
    shown = pic.copy()
    shown.px = [[None if v in (0, None) else v for v in row] for row in pic.px]
    return shown.image({1: palette[1], 2: palette[2], 3: palette[3]})


plane = list(OBJ_FLIGHT[0])
sprite_image(S.PLANE, plane).save(ART / 'plane-native.png')
LAB_NAMES = ['grok', 'claude', 'gemini', 'muse']
fleet_strips = []
for lab in range(4):
    d, g, h = E.fleet(lab)
    body, accent = LABS[lab][0], LABS[lab][1]
    parts = [sprite_image(p, body) for p in d] + [sprite_image(p, accent) for p in g] + [sprite_image(p, body) for p in h]
    strip = Image.new('RGBA', (sum(p.width + 4 for p in parts), 16), (0, 0, 0, 0))
    x = 0
    for p in parts:
        strip.paste(p, (x, 0), p)
        x += p.width + 4
    strip.save(ART / f'{LAB_NAMES[lab]}-native.png')
    fleet_strips.append(strip)
for i, im in enumerate(boss_images):
    im.save(ART / f'boss-{i + 1}-native.png')
flight = [(S.PLANE, plane), (S.PLANE_BANK, plane), (S.FLAME[0], OBJ_FLIGHT[5]), (S.FLAME[1], OBJ_FLIGHT[5]),
          (S.SHOT, OBJ_FLIGHT[1]), (S.SHOT_L, OBJ_FLIGHT[1]), (S.SHOT_SOLO, OBJ_FLIGHT[1]),
          (S.ALLY[0], plane), (S.CORE, OBJ_FLIGHT[1])]
flight += [(b, LABS[k][2]) for k, b in enumerate((S.BULLET_SMALL, S.BULLET_BIG, S.BULLET_NEEDLE, S.BULLET_STAR))]
flight += [(c, OBJ_FLIGHT[6]) for c in S.COIN] + [(S.POWER, OBJ_FLIGHT[6]), (S.REPAIR, OBJ_FLIGHT[7]), (S.BOLT, OBJ_FLIGHT[1])]
flight += [(e, OBJ_FLIGHT[5]) for e in S.EXPLOSION] + [(S.SPARK[0], OBJ_FLIGHT[5]), (S.RING, OBJ_FLIGHT[1])]
flight_images = [sprite_image(p, c) for p, c in flight]
width = max(sum(p.width + 4 for p in flight_images[:13]), 4 * 100)
sheet = Image.new('RGB', (width + 8, 8 + 2 * 20 + 4 * 20 + 60 + 4), hexrgb(M.BACKDROP))
x, y = 4, 4
for n, p in enumerate(flight_images):
    if n == 13:
        x, y = 4, y + 20
    sheet.paste(p, (x, y + (16 - p.height) // 2), p)
    x += p.width + 4
for lab, strip in enumerate(fleet_strips):
    sheet.paste(strip, (4, 44 + lab * 20), strip)
for i, im in enumerate(boss_images):
    sheet.paste(im, (4 + i * 100, 124))
sheet.resize((sheet.width * 3, sheet.height * 3), Image.Resampling.NEAREST).save(ART / 'native-contact-sheet.png')
print('bg common', len(bg), 'obj', len(obj) // 16, '+fleet', FLEET_TILES, '+core', len(core_data) // 16,
      'menu obj', len(menu_obj) // 16)
for i, (n, p, q) in enumerate(sector_info):
    print(f'sector {i + 1}: {n} tiles, {p} placements, {q} quantised tiles')
for i, (n, q) in enumerate(boss_info):
    print(f'boss {i + 1}: {n} tiles, {q} quantised tiles')
