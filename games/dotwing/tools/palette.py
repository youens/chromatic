"""Dotwing colour language: cobalt skies, ivory airframes, gold tokens."""

INK = '#0a1030'
NAVY = '#152052'
COBALT = '#23409c'
SKY = '#3a78dc'
SKYL = '#7cbcf8'
ICE = '#c8ecff'
WHITE = '#ffffff'
IVORY = '#fff2d6'
CYAN = '#5cf2ee'
TEAL = '#1aa0b4'
MINT = '#5ee8b0'
GOLD = '#ffc83a'
GOLDL = '#fff1a0'
AMBER = '#e2801e'
BROWN = '#7a3c1c'
RED = '#e2324a'
PINK = '#ff5aa0'
CORAL = '#ff7a5a'
PEACH = '#ffc4a0'
LILAC = '#bc9bff'
PURPLE = '#8a4cf0'
GREEN = '#46c25a'

# Pilot livery colours, in save-file order: PINK BLUE MINT GOLD LILAC CYAN.
LIVERY = ['#ff7eb8', '#6aa8ff', '#52e0a6', '#ffc84a', '#b48cff', '#56e2f0']
LIVERY_NAMES = ['PINK', 'BLUE', 'MINT', 'GOLD', 'LILAC', 'CYAN']

# Sprite palettes during flight (index 0 is transparent and never shown).
OBJ_FLIGHT = [
    [INK, INK, LIVERY[2], IVORY],         # 0 plane, ally dots (livery swapped in)
    [INK, '#1a6ad8', CYAN, WHITE],        # 1 player energy: shots, core, burst
    [INK, INK, PINK, WHITE],              # 2 hostile bullets (per sector)
    [INK, INK, '#8090b0', WHITE],         # 3 rival craft body (per sector)
    [INK, INK, '#ff6a50', GOLDL],         # 4 rival craft accent (per sector)
    [INK, '#c2281e', '#ff9a2a', GOLDL],   # 5 fire: explosions, flame, hit flash
    [INK, BROWN, GOLD, GOLDL],            # 6 gold: tokens, power
    [INK, '#0e1840', '#3ad070', WHITE],   # 7 shadow + repair
]

# Rival lab skins: (body, accent, bullet) palettes; index 0 unused.
LABS = [
    # Grok: graphite and silver with a white slash
    ([INK, INK, '#6e7894', '#e8eefc'], [INK, INK, '#c8d0e8', WHITE], [INK, '#9a1060', '#ff4aa0', WHITE]),
    # Claude: coral sunburst on cream
    ([INK, '#7a2a1a', '#ff7a5a', '#ffe0c4'], [INK, INK, '#ffb070', '#fff6e8'], [INK, '#0a4a6a', '#5cf2ee', WHITE]),
    # Gemini: indigo crystal and cyan sparkle
    ([INK, INK, '#4a6cff', '#b8e8ff'], [INK, INK, '#8a5cff', '#f0e8ff'], [INK, '#a01848', '#ff5a7a', '#fff0a0']),
    # Muse: violet M fleet
    ([INK, INK, '#9a4cf0', '#f0d8ff'], [INK, INK, '#ff5ac8', '#ffe0f4'], [INK, '#2a7a10', '#b8ff4a', WHITE]),
]
