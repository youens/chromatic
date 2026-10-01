"""Assemble the static game collection from the checked-in release ROMs."""
from pathlib import Path
import html
import json
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'collection/dist'
GAMES = json.loads((ROOT / 'tools/games.json').read_text())
COUNT = len(GAMES)
WORD = str(COUNT)
CARTRIDGE = ROOT / 'games/arcade/dist/chromatic-arcade.gbc'
BASE = 'https://chromatic.youens.com'
COLLECTION_IMAGE = '/chromatic-fun-share-v3.jpg'
COLLECTION_IMAGE_ALT = 'Chromatic Fun by Youens: One cartridge, many games'
def cover_kind(game):
    return 'pixel-art' if game.get('cover') == 'pixel' else 'illustrated'
OPTIMIZED = {'dot-swarm', 'stormkite', 'comet-links', 'prism-well', 'dreambase-invaders'}
def artfile(slug, kind='card'):
    if slug in OPTIMIZED:
        return f'{slug}-{kind}.jpg' if kind == 'share' else f'{slug}-{kind}.webp'
    return slug + ('.png' if slug in ('hello-dot','dot-swarm','stormkite','comet-links','prism-well','dreambase-invaders') else '.jpg')
if OUT.exists():
    shutil.rmtree(OUT)
OUT.mkdir(parents=True, exist_ok=True)
for folder in ('art','roms','screens','vendor'):
    (OUT / folder).mkdir(exist_ok=True)
for source in (ROOT / 'collection/web').glob('*'):
    if source.is_file(): shutil.copy2(source, OUT / source.name)
for name in ('binjgb.js','binjgb.wasm','LICENSE'):
    shutil.copy2(ROOT / 'games/hello-dot/web/vendor' / name, OUT / 'vendor' / name)
for g in GAMES:
    slug = g['slug']; game = ROOT / 'games' / slug
    rom = game / 'dist' / f'{slug}.gbc'
    data = rom.read_bytes()
    assert len(data) == 32768 and data[0x143] == 0xC0, f'Invalid release: {slug}'
    shutil.copy2(rom, OUT / 'roms' / rom.name)
    art = game / g.get('art', 'web/images/hello-dot-share-v1.png' if slug == 'hello-dot' else 'art/cover.jpg')
    if slug in OPTIMIZED:
        for kind in ('card', 'detail', 'share'):
            ext = 'jpg' if kind == 'share' else 'webp'
            shutil.copy2(game / 'art' / f'cover-{kind}-v1.{ext}', OUT / 'art' / artfile(slug, kind))
    else:
        shutil.copy2(art, OUT / 'art' / artfile(slug))
    screenshot = game / ('docs/screenshots/gameplay.png' if slug == 'hello-dot' else 'art/gameplay.png')
    shutil.copy2(screenshot, OUT / 'screens' / f'{slug}.png')
cart = CARTRIDGE.read_bytes()
assert len(cart) == 524288 and cart[0x147] == 0x1B, 'Invalid anthology cartridge'
shutil.copy2(CARTRIDGE, OUT / 'roms' / CARTRIDGE.name)
for name in ('launcher-shelf', 'launcher-intro'):
    shutil.copy2(ROOT / 'games/arcade/art' / f'{name}.png', OUT / 'screens' / f'{name}.png')
with zipfile.ZipFile(OUT / 'chromatic-arcade-roms.zip','w',zipfile.ZIP_DEFLATED) as archive:
    for g in GAMES:
        archive.write(OUT / 'roms' / (g['slug']+'.gbc'),g['slug']+'.gbc')
    archive.write(CARTRIDGE, CARTRIDGE.name)
    archive.write(ROOT / 'games/hello-dot/licenses/GBDK.txt','licenses/GBDK.txt')

NAV='''<header><a class="brand" href="/"><span class="brandmark" aria-hidden="true"><i></i><i></i><i></i></span>CHROMATIC FUN<span class="tiny-label">BY YOUENS</span></a><nav class="navlinks"><a href="/#games">The games</a><a href="/#about">The project</a><span class="pill"><i class="status-dot"></i>'''+str(COUNT)+''' worlds. Ready to play.</span></nav></header>'''
FOOT='''<footer class="footer"><span>Small games. Big imagination. Made by Youens.</span><span>Independent homebrew collection. Not affiliated with ModRetro or OpenAI.</span></footer>'''
def head(title, desc, image=None, url='/'):
    image_path = COLLECTION_IMAGE if image is None else '/art/' + artfile(image, 'share')
    image_alt = COLLECTION_IMAGE_ALT if image is None else title + ' game cover'
    image_details = '<meta property="og:image:type" content="image/jpeg"><meta property="og:image:width" content="1200"><meta property="og:image:height" content="628">' if image is None else ''
    return f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#090d14"><title>{html.escape(title)}</title><meta name="description" content="{html.escape(desc,quote=True)}"><link rel="canonical" href="{BASE}{url}"><meta property="og:type" content="website"><meta property="og:site_name" content="Chromatic Fun by Youens"><meta property="og:title" content="{html.escape(title,quote=True)}"><meta property="og:description" content="{html.escape(desc,quote=True)}"><meta property="og:url" content="{BASE}{url}"><meta property="og:image" content="{BASE}{image_path}"><meta property="og:image:alt" content="{html.escape(image_alt,quote=True)}">{image_details}<meta name="twitter:card" content="summary_large_image"><meta name="twitter:image" content="{BASE}{image_path}"><meta name="twitter:image:alt" content="{html.escape(image_alt,quote=True)}"><meta name="twitter:title" content="{html.escape(title,quote=True)}"><meta name="twitter:description" content="{html.escape(desc,quote=True)}"><link rel="stylesheet" href="/style.css"></head>'''

cards=[]
for g in GAMES:
    slug=g['slug']; category=g['category']
    cards.append(f'''<article class="card" data-category="{category}" style="--accent:{g['accent']}"><a class="card-art" href="/games/{slug}/" aria-label="Play {g['title']}"><img src="/art/{artfile(slug)}" alt="{g['title']} {cover_kind(g)} cover" width="1536" height="1024" loading="lazy"><span class="play-tag">PLAY GAME ↗</span></a><div class="card-title"><h3><a href="/games/{slug}/">{g['title']}</a></h3><span>{g['genre']}</span></div><p>{g['tagline']}</p></article>''')
index=head('Chromatic Fun by Youens · One cartridge, many games',f'{WORD.title()} original Game Boy Color games. Play instantly in your browser or download real cartridges for ModRetro Chromatic.')+f'''<body><div class="shell">{NAV}<main><section class="hero"><div><p class="eyebrow">AN INDEPENDENT POCKET ARCADE</p><h1>One cartridge.<br><span>Many games.</span></h1><p class="lede">Neon highways. Lunar ruins. Storm kites and gravity golf. Tiny games with a whole lot going on, built for the Game Boy Color in your hands.</p><div class="hero-actions"><a class="button" href="#games">Find your next obsession <span>↓</span></a><a class="button secondary" href="/games/chromatic-arcade/">Play the whole cartridge</a><a class="button secondary" href="/chromatic-arcade-roms.zip" download>Get all {COUNT} ROMs</a></div></div><a class="feature" href="/games/chromatic-arcade/" aria-label="Play the full collection"><img src="{COLLECTION_IMAGE}" alt="{COLLECTION_IMAGE_ALT}" width="1200" height="628" fetchpriority="high"><div class="feature-label"><div><span class="tiny-label">THE FULL COLLECTION</span><strong>Open the game menu</strong></div><span class="arrow" aria-hidden="true">↗</span></div></a></section><div class="spec-strip"><span><b>{COUNT:02}</b> ORIGINAL GAMES</span><span><b>160 × 144</b> PIXELS OF POSSIBILITY</span><span><b>REAL GBC</b> ROMS</span><span><b>ZERO</b> INSTALLS TO PLAY</span></div><section class="library" id="games"><div class="section-head"><h2>Pick a world.<small id="game-count">{COUNT:02} GAMES</small></h2><div class="filters" aria-label="Filter games"><button data-filter="all" aria-pressed="true">All games</button><button data-filter="action" aria-pressed="false">Action</button><button data-filter="exploration" aria-pressed="false">Exploration</button><button data-filter="puzzle" aria-pressed="false">Puzzle</button></div></div><div class="games">{''.join(cards)}</div></section><section class="info-band" id="about"><div><p class="eyebrow">SMALL HARDWARE. BIG IDEAS.</p><h2>One cartridge, many games</h2><p><strong>Start + Select brings you home from any game.</strong> Browse the cartridge shelf, press A to play, and return whenever you like.</p><p>Every game is a real, downloadable Game Boy Color ROM. The browser runs that same cartridge in an emulator. Take it to a compatible flash cartridge and play on your ModRetro Chromatic, or flash the single anthology cartridge: every game behind one launcher, with best scores saved to the cartridge.</p></div><div><p class="eyebrow">A LITTLE LABORATORY FOR PLAY</p><h2>Many ways to push a button.</h2><p>Perspective roads, reversible gravity, procedural stealth, living circuits, a musical orbit, scanline parallax, gravity golf and cascading crystals. Each game has its own pixel artwork, sound, and rules. The covers are illustrated key art; open a game to see actual cartridge pixels. The collection is built for ModRetro Chromatic.</p></div></section></main>{FOOT}</div><script src="/library.js"></script></body></html>'''
(OUT/'index.html').write_text(index)
ANTHOLOGY={'slug':'chromatic-arcade','title':'Chromatic Fun by Youens','genre':'One cartridge, many games','description':f'All {WORD} games on one Game Boy Color cartridge, behind a launcher of its own. Slide along the shelf, read a game\'s card, then press A to play. Hold Start and Select together in any game to return to the shelf.','accent':'#a4e8da','controls':[['← →','Browse the shelf'],['A / X / Space','Play'],['B / Z','Game card'],['Start + Select','Return to the launcher']],'goal':'Best scores and play counts are saved to cartridge memory; in the browser they last until the page reloads. The Select button on the shelf opens the Hall of Light.','experiment':'A 512 KiB MBC5 cartridge with battery-backed RAM. Each game keeps its own code and graphics banks; the launcher, shared runtime and interrupt handlers live in the fixed bank. The shelf slides on its own scanline split while the header and details stay fixed.','order':0}
for g in GAMES+[ANTHOLOGY]:
    slug=g['slug']; anthology=slug=='chromatic-arcade'; rows=''.join(f'<div class="control-row"><kbd>{html.escape(a)}</kbd><span>{html.escape(b)}</span></div>' for a,b in g['controls'])
    system=('<div class="control-row"><kbd>Enter</kbd><span>Start</span></div><div class="control-row"><kbd>C</kbd><span>Select</span></div><div class="control-row"><kbd>M</kbd><span>Sound</span></div>' if anthology else '''<div class="control-row"><kbd>Enter / Start</kbd><span>Start or pause</span></div><div class="control-row"><kbd>M / Select</kbd><span>Sound</span></div>''')
    controls=f'''<section class="controls"><h2>THE CONTROLS</h2>{rows}{system}<p class="goal">{g['goal']}</p></section>'''
    page=head(g['title']+' · Chromatic Fun by Youens',g['description'],None if anthology else slug,f'/games/{slug}/')+f'''<body class="game-page" data-game="{slug}" data-title="{g['title']}" style="--accent:{g['accent']}"><div class="shell">{NAV}<main><div class="breadcrumb"><a href="/#games">← All {WORD} worlds</a></div><div class="game-layout"><section class="game-info"><img class="mini-cover" src="{'/screens/launcher-intro.png' if anthology else '/art/'+artfile(slug, 'detail')}" alt="{g['title']} {'title screen' if anthology else cover_kind(g)+' cover'}"><p class="eyebrow">{g['genre']}</p><h1>{g['title']}</h1><p class="lede">{g['description']}</p><a class="button secondary" href="/roms/{slug}.gbc" download>{'Download the cartridge ROM' if anthology else 'Download GBC ROM'} <span>↓</span></a>{controls}<div class="screenshots"><img src="/screens/{'launcher-shelf' if anthology else slug}.png" alt="Actual {g['title']} {'launcher' if anthology else 'gameplay'}"><p>Actual cartridge pixels.<br>160 × 144. Every one counts.</p></div></section><section class="game-player" aria-label="{g['title']} player"><div class="device"><div class="device-top"><span>CHROMATIC / {'ALL' if anthology else format(g['order'],'02')}</span><span>COLOR SYSTEM</span></div><div class="screen-wrap"><canvas id="screen" width="160" height="144" tabindex="0" aria-label="{g['title']} game. Use arrow keys and X or Space for A, Z for B, Enter to pause."></canvas><button class="launch" id="launch" disabled><span class="play-circle">▶</span><strong>Loading your cartridge…</strong><small>The real ROM, right in your browser.</small></button></div><div class="device-bar"><span id="status" role="status">LOADING</span><div class="device-tools"><button id="sound" aria-pressed="false">SOUND OFF</button><button id="pause" disabled>START / PAUSE</button>{'<button id="menu" disabled>START + SELECT</button>' if anthology else ''}<button id="fullscreen" aria-label="Fullscreen game">⛶</button><button id="reset" aria-label="Restart cartridge">↻</button></div></div></div><div class="touch-controls" aria-label="Game controls"><div class="dpad"><button data-key="up" aria-label="Move up">↑</button><button data-key="left" aria-label="Move left">←</button><span></span><button data-key="right" aria-label="Move right">→</button><button data-key="down" aria-label="Move down">↓</button></div><div class="action-buttons"><button data-key="B"><b>B</b><span>Z / SHIFT</span></button><button data-key="A"><b>A</b><span>X / SPACE</span></button></div></div><p class="game-caption">Keyboard, touch, or connected gamepad. Your next round is waiting.</p></section></div><section class="game-extra"><h2>Inside the cartridge</h2><p>{g['experiment']} {'Every game runs unchanged from its own banks.' if anthology else 'Native pixel artwork and synthesized sound, running from a 32 KiB Color-only cartridge. High scores last for this power-on session.'} </p></section></main>{FOOT}</div><script src="/vendor/binjgb.js"></script><script src="/player.js"></script></body></html>'''
    dest=OUT/'games'/slug;dest.mkdir(parents=True,exist_ok=True);(dest/'index.html').write_text(page)
(OUT/'_headers').write_text('''/roms/*
  Content-Type: application/octet-stream
  Content-Disposition: attachment

/vendor/binjgb.wasm
  Content-Type: application/wasm

/chromatic-arcade-roms.zip
  Content-Type: application/zip
  Content-Disposition: attachment; filename="chromatic-arcade-roms.zip"
''')
print(f'Built {COUNT}-game collection at',OUT)
