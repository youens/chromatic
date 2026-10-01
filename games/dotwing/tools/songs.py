"""Dotwing's soundtrack and sound effects, in the MML dialect of music.py.

Every bar starts with an explicit octave so bars can be read alone. Song
order matches the SONG_ enum in src/dotwing.h; effects match SFX_.
"""


def x(note):
    """Square-channel frequency register value for a MIDI note."""
    f = 440 * 2 ** ((note - 69) / 12)
    return max(0, min(2047, round(2048 - 131072 / f)))


def tone(frames, note, env=0xF1, duty=2, sweep=0):
    v = x(note)
    return (frames, sweep, duty << 6, env, v & 255, 0x80 | (v >> 8))


def noise(frames, env, poly):
    return (frames, 0, env, poly, 0x80, 0)


DRUMS_DRIVE = '[k8 h8 s8 h8 k8 k8 s8 h8]'

TITLE = {
    't': 6,
    'sq2': '''@0 !
        o5 g4. e8 g4 o6 c4 | o5 b4. a8 g4 d4 | o5 e4. c8 e4 a4 | o5 g4 f8 e8 f2
        o5 g4. e8 g4 o6 c4 | o6 d4. c8 o5 b4 o6 d4 | o6 c8 o5 a8 f8 a8 b8 g8 d8 g8 | o6 c2. r4
        @2 o5 a4 o6 c8 e8 d4 c4 | o5 a4 o6 c8 d8 c4 o5 a4 | o5 g4 e8 g8 o6 c4 e4 | o6 d4. e8 d4 o5 b4
        o6 c4 o5 e8 a8 g4 e4 | o5 f4 a8 o6 c8 d4 c4 | o5 b4 o6 d8 c8 o5 b8 a8 g8 b8 | o6 c4 o5 g8 e8 c4 r4 @0''',
    'sq1': '''@3 !
        [o4 c16 e16 g16 o5 c16]4 [o4 d16 g16 b16 o5 d16]4 [o4 c16 e16 a16 o5 c16]4 [o4 c16 f16 a16 o5 c16]4
        [o4 c16 e16 g16 o5 c16]4 [o4 d16 g16 b16 o5 d16]4 [o4 c16 f16 a16 o5 c16]2 [o4 d16 g16 b16 o5 d16]2
        [o4 c16 e16 g16 o5 c16]4
        [o4 c16 e16 a16 o5 c16]4 [o4 c16 f16 a16 o5 c16]4 [o4 c16 e16 g16 o5 c16]4 [o4 d16 g16 b16 o5 d16]4
        [o4 c16 e16 a16 o5 c16]4 [o4 c16 f16 a16 o5 c16]4 [o4 d16 g16 b16 o5 d16]4 [o4 c16 e16 g16 o5 c16]4''',
    'wave': '''@5 !
        [o3 c8 o4 c8]4 [o3 g8 o4 g8]4 [o3 a8 o4 a8]4 [o3 f8 o4 f8]4
        [o3 c8 o4 c8]4 [o3 g8 o4 g8]4 [o3 f8 o4 f8]2 [o3 g8 o4 g8]2 [o3 c8 o4 c8]4
        [o3 a8 o4 a8]4 [o3 f8 o4 f8]4 [o3 c8 o4 c8]4 [o3 g8 o4 g8]4
        [o3 a8 o4 a8]4 [o3 f8 o4 f8]4 [o3 g8 o4 g8]4 [o3 c8 o4 c8]4''',
    'noise': '! ' + DRUMS_DRIVE + '7 k8 h8 s8 h8 s16 s16 s16 s16 x8 h8 ' + DRUMS_DRIVE + '7 k8 k8 s8 s8 s16 s16 s16 s16 x4',
}

FLIGHT_A = {
    't': 6,
    'sq2': '''@0 !
        o5 e8 a8 o6 c8 e8 d8 c8 o5 b8 a8 | o6 c4 o5 a4 f8 a8 o6 c8 d8 | o6 e4. d8 c8 d8 e8 g8 | o6 d4 o5 b4 o6 d8 e8 d8 o5 b8
        o5 e8 a8 o6 c8 e8 d8 c8 o5 b8 a8 | o6 c4 o5 a4 f8 a8 o6 c8 f8 | o6 e4. d8 c4 o5 b4 | o5 a2 r4 e8 g8
        @4 o5 a4. g8 f4 a4 | o5 g4. f8 e4 g4 | o5 e4 g8 b8 o6 d4 c8 o5 b8 | o5 a2. r4
        o5 f8 a8 o6 c8 f8 e8 c8 o5 a8 o6 c8 | o5 g8 b8 o6 d8 g8 f8 d8 o5 b8 o6 d8 | o6 e4. d8 c8 o5 b8 a8 g8 | o5 a2 r2 @0''',
    'sq1': '''@3 !
        [o4 a16 o5 c16 e16 c16]4 [o4 a16 o5 c16 f16 c16]4 [o4 g16 o5 c16 e16 c16]4 [o4 g16 b16 o5 d16 o4 b16]4
        [o4 a16 o5 c16 e16 c16]4 [o4 a16 o5 c16 f16 c16]4 [o4 g16 b16 o5 d16 o4 b16]4 [o4 a16 o5 c16 e16 c16]4
        [o4 a16 o5 c16 f16 c16]4 [o4 g16 b16 o5 d16 o4 b16]4 [o4 g16 b16 o5 e16 o4 b16]4 [o4 a16 o5 c16 e16 c16]4
        [o4 a16 o5 c16 f16 c16]4 [o4 g16 b16 o5 d16 o4 b16]4 [o4 a16 o5 c16 e16 c16]4 [o4 a16 o5 c16 e16 c16]4''',
    'wave': '''@6 !
        [o3 a8 a8 o4 a8 o3 a8]2 [o3 f8 f8 o4 f8 o3 f8]2 [o3 c8 c8 o4 c8 o3 c8]2 [o3 g8 g8 o4 g8 o3 g8]2
        [o3 a8 a8 o4 a8 o3 a8]2 [o3 f8 f8 o4 f8 o3 f8]2 [o3 g8 g8 o4 g8 o3 g8]2 [o3 a8 a8 o4 a8 o3 a8]2
        [o3 f8 f8 o4 f8 o3 f8]2 [o3 g8 g8 o4 g8 o3 g8]2 [o3 e8 e8 o4 e8 o3 e8]2 [o3 a8 a8 o4 a8 o3 a8]2
        [o3 f8 f8 o4 f8 o3 f8]2 [o3 g8 g8 o4 g8 o3 g8]2 [o3 a8 a8 o4 a8 o3 a8]2 [o3 a8 o4 a8 o3 a8 o4 a8]2''',
    'noise': '! [' + '[k8 h8 s8 h8]2 ' * 3 + 'k8 h8 s8 h8 k16 k16 s8 s16 s16 s8]4',
}

FLIGHT_B = {
    't': 5,
    'sq2': '''@4 !
        o5 d8 f8 a8 o6 d8 c8 o5 a8 f8 a8 | o5 b-4 a8 g8 f4 d4 | o5 e8 g8 o6 c8 e8 d8 c8 o5 b-8 g8 | o5 a4 o6 c+8 e8 a4 r4
        o5 d8 f8 a8 o6 d8 c8 o5 a8 f8 a8 | o5 b-8 o6 d8 f8 d8 c8 o5 b-8 a8 g8 | o6 c4 e4 g4 e4 | o5 a2 o6 c+4 e4
        @0 o6 f4. e8 d4 c4 | o6 d4. c8 o5 b-4 a4 | o5 g8 b-8 o6 d8 g8 f8 e8 d8 c8 | o5 a2 r4 a8 o6 c+8
        o6 d8 f8 a8 f8 d8 f8 a8 o7 d8 | o6 b-8 o7 d8 f8 d8 c8 o6 b-8 a8 g8 | o6 g8 e8 c8 e8 g8 o7 c8 o6 b-8 g8 | o6 a2. r4 @4''',
    'sq1': '''@3 !
        [o4 d16 f16 a16 f16]4 [o4 d16 f16 b-16 f16]4 [o4 e16 g16 o5 c16 o4 g16]4 [o4 e16 a16 o5 c+16 o4 a16]4
        [o4 d16 f16 a16 f16]4 [o4 d16 f16 b-16 f16]4 [o4 e16 g16 o5 c16 o4 g16]4 [o4 e16 a16 o5 c+16 o4 a16]4
        [o4 d16 f16 a16 f16]4 [o4 d16 f16 b-16 f16]4 [o4 e16 g16 o5 c16 o4 g16]4 [o4 e16 a16 o5 c+16 o4 a16]4
        [o4 d16 f16 a16 f16]4 [o4 d16 f16 b-16 f16]4 [o4 e16 g16 o5 c16 o4 g16]4 [o4 e16 a16 o5 c+16 o4 a16]4''',
    'wave': '''@6 !
        [[o3 d8 o4 d8]4 [o2 b-8 o3 b-8]4 [o3 c8 o4 c8]4 [o2 a8 o3 a8]4]4''',
    'noise': '! [[k16 k16 h8 s8 h8]3 k16 k16 s16 s16 s8 s8]8',
}

BOSS = {
    't': 5,
    'sq2': '''@0 !
        o5 e8 e8 g8 e8 f+8 e8 d+8 e8 | o5 c8 c8 e8 c8 d8 c8 o4 b8 o5 c8 | o5 a8 a8 o6 c8 o5 a8 b8 a8 g+8 a8 | o5 b4 o6 d+4 f+4 a4
        o6 e4. d+8 e8 g8 f+8 e8 | o6 c4. o5 b8 a8 g8 f+8 g8 | o5 a8 o6 c8 e8 a8 g8 e8 c8 o5 a8 | o5 b2 a+4 b4''',
    'sq1': '''@8 !
        [o4 e16 g16 b16 g16]4 [o4 e16 g16 o5 c16 o4 g16]4 [o4 e16 a16 o5 c16 o4 a16]4 [o4 d+16 f+16 b16 f+16]4
        [o4 e16 g16 b16 g16]4 [o4 e16 g16 o5 c16 o4 g16]4 [o4 e16 a16 o5 c16 o4 a16]4 [o4 d+16 f+16 b16 f+16]4''',
    'wave': '''@6 !
        [o3 e8 e8 o4 e8 o3 e8]2 [o3 c8 c8 o4 c8 o3 c8]2 [o3 a8 a8 o4 a8 o3 a8]2 [o3 b8 b8 o4 b8 o3 b8]2
        [o3 e8 e8 o4 e8 o3 e8]2 [o3 c8 c8 o4 c8 o3 c8]2 [o3 a8 a8 o4 a8 o3 a8]2 [o3 b8 o4 b8 o3 b8 o4 b8]2''',
    'noise': '! [[k16 k16 h16 h16 s8 h8]3 k16 k16 s16 s16 s16 s16 s8]4',
}

HANGAR = {
    't': 8,
    'sq2': '''@2 !
        o5 e4 g4 b4 a8 g8 | o5 e4 c4 r4 e8 g8 | o5 f4 a4 o6 c4 o5 b8 a8 | o5 g2 r4 d8 e8
        o5 e4 g4 b4 o6 d8 c8 | o5 a4 e4 c4 e8 g8 | o5 f8 a8 o6 c8 o5 a8 g8 f8 e8 d8 | o5 c2 r2''',
    'sq1': '''@8 !
        [o4 e8 g8 b8 g8]2 [o4 c8 e8 g8 e8]2 [o4 d8 f8 a8 f8]2 [o4 d8 f8 g8 b8]2
        [o4 e8 g8 b8 g8]2 [o4 c8 e8 g8 e8]2 [o4 d8 f8 a8 f8]1 [o4 d8 f8 g8 b8]1 [o4 c8 e8 g8 e8]2''',
    'wave': '''@7 !
        o3 c4 g4 e4 g4 | o3 a4 e4 c4 e4 | o3 d4 a4 f4 a4 | o3 g4 d4 b4 d4
        o3 c4 g4 e4 g4 | o3 a4 e4 c4 e4 | o3 d4 a4 o3 g4 d4 | o3 c4 g4 c2''',
    'noise': '! [k4 h8 h8 s4 h8 h8]8',
}

CLEAR = {
    't': 5,
    'sq2': '@4 o5 c8 e8 g8 o6 c4 o5 g8 o6 c8 e8 g2.',
    'sq1': '@1 o4 g8 o5 c8 e8 g4 e8 g8 o6 c8 e2.',
    'wave': '@5 o3 c4 g4 o4 c4 o3 g4 c2.',
    'noise': 'k8 s8 k8 s8 k8 s8 s16 s16 s16 s16 x2.',
}

VICTORY = {
    't': 6,
    'sq2': '''@4 o5 g4. e8 g4 o6 c4 | o5 b4. a8 g4 o6 d4 | o6 e4. d8 c4 e4 | o6 g2 f4 e4
        o6 d4. c8 o5 b4 o6 d4 | o6 c8 o5 a8 f8 a8 b8 g8 d8 g8 | o6 c1 & c2 r2''',
    'sq1': '''@1 o5 e4. c8 e4 g4 | o5 d4. c8 o4 b4 o5 b4 | o5 c4. g8 e4 g4 | o5 b2 a4 g4
        o5 f4. e8 d4 f4 | o5 a8 f8 c8 f8 g8 d8 o4 b8 o5 d8 | o5 e1 & e2 r2''',
    'wave': '''@5 [o3 c8 o4 c8]4 [o3 g8 o4 g8]4 [o3 a8 o4 a8]4 [o3 e8 o4 e8]4
        [o3 f8 o4 f8]4 [o3 f8 o4 f8]2 [o3 g8 o4 g8]2 o3 c1 & c2 r2''',
    'noise': '[k8 h8 s8 h8]13 x1 r2',
}

OVER = {
    't': 8,
    'sq2': '@2 o5 e4 d+4 d4 c+4 | o5 c2. r4',
    'sq1': '@1 o4 g4 f+4 f4 e4 | o4 e2. r4',
    'wave': '@7 o3 a2 g+2 | o3 a2. r4',
}

SONGS = [('title', TITLE), ('hangar', HANGAR), ('flight_a', FLIGHT_A), ('flight_b', FLIGHT_B),
         ('boss', BOSS), ('clear', CLEAR), ('victory', VICTORY), ('over', OVER)]

# (name, channel 1 or 4, priority, steps). Square steps are
# (frames, NR10, NR11, NR12, freq lo, freq hi|trigger); noise steps are
# (frames, 0, NR42, NR43, 0x80, 0).
SFX = [
    ('shot', 1, 1, [tone(4, 84, 0x51, 2, 0x17)]),
    ('hit', 4, 2, [noise(3, 0x61, 0x22)]),
    ('pop', 4, 3, [noise(6, 0xA2, 0x34), noise(6, 0x72, 0x45)]),
    ('boom', 4, 4, [noise(8, 0xC3, 0x56), noise(12, 0x93, 0x67)]),
    ('coin', 1, 3, [tone(4, 88, 0xB1, 2), tone(10, 95, 0xC3, 2)]),
    ('power', 1, 4, [tone(4, 72, 0xC2, 2), tone(4, 76, 0xC2, 2), tone(4, 79, 0xC2, 2), tone(10, 84, 0xD4, 2)]),
    ('hurt', 4, 6, [noise(6, 0xF2, 0x71), noise(16, 0xC4, 0x75)]),
    ('burst', 1, 6, [tone(28, 60, 0xF5, 2, 0x26)]),
    ('move', 1, 2, [tone(3, 81, 0x81, 1)]),
    ('ok', 1, 3, [tone(4, 79, 0xA1, 2), tone(8, 86, 0xB3, 2)]),
    ('no', 1, 3, [tone(6, 55, 0xA2, 1), tone(10, 52, 0xA3, 1)]),
    ('buy', 1, 4, [tone(4, 76, 0xC2, 2), tone(4, 81, 0xC2, 2), tone(4, 88, 0xC2, 2), tone(14, 93, 0xD5, 2)]),
    ('warn', 1, 5, [tone(10, 69, 0xF0, 2), tone(10, 63, 0xF0, 2), tone(10, 69, 0xF0, 2), tone(14, 63, 0xF3, 2)]),
    ('bigboom', 4, 7, [noise(10, 0xF4, 0x57), noise(20, 0xF6, 0x67), noise(20, 0xA7, 0x77)]),
    ('launch', 1, 5, [tone(24, 50, 0xC4, 2, 0x15)]),
]
