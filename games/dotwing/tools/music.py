#!/usr/bin/env python3
"""Compile Dotwing's music and sound effects into src/music_data.h.

Songs are written in a small MML dialect, one string per channel:

  t6        frames per sixteenth note (sets tempo for the whole song)
  o4 > <    octave, up, down          l8   default note length (1,2,4,8,16)
  c d e f g a b, with + or - for sharps/flats, optional length and dots
  r         rest        &   tie (extend the previous note)
  @n        instrument  [ ... ]n  repeat n times    !  loop point    |  bar line (ignored)
  Drum channel letters: k kick, s snare, h hat, o open hat, x crash, t tom

Stream bytes: 0x00 rest, 0x01-0x7F note, 0x80|(len-1) length in frames,
0xC0 tie, 0xC1 n instrument, 0xC3 lo hi jump, 0xC6 n repeat start,
0xC7 repeat end, 0xFF end.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

# Instruments: (channel kind, duty or wave, envelope, gate frames, volume)
INSTRUMENTS = [
    ('sq', 2, 0xA3, 0, 0),     # 0 lead: 50% duty, bright decay
    ('sq', 1, 0x72, 5, 0),     # 1 pluck: 25%, short
    ('sq', 2, 0x87, 0, 0),     # 2 long lead
    ('sq', 0, 0x61, 3, 0),     # 3 arp: 12.5%, staccato
    ('sq', 1, 0xB4, 0, 0),     # 4 brass
    ('wv', 0, 0, 0, 0x20),     # 5 bass, full volume, triangle-ish
    ('wv', 1, 0, 5, 0x20),     # 6 punchy bass, saw
    ('wv', 2, 0, 0, 0x40),     # 7 soft pad
    ('sq', 3, 0x91, 4, 0),     # 8 thin echo
    ('sq', 2, 0x58, 0, 0),     # 9 swell (envelope rises)
]
WAVES = [
    [0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10],
    [0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF],
    [0x02, 0x46, 0x8A, 0xCE, 0xFF, 0xFE, 0xEE, 0xDD, 0xCB, 0xA9, 0x87, 0x65, 0x43, 0x21, 0x00, 0x00],
]
# Drum kit: (NR42 envelope, NR43 polynomial counter)
DRUMS = {'k': (0xA1, 0x63), 's': (0x92, 0x34), 'h': (0x41, 0x10), 'o': (0x53, 0x12),
         'x': (0xC4, 0x45), 't': (0x82, 0x5C)}
DRUM_IDS = {k: i + 1 for i, k in enumerate('kshoxt')}
NOTES = {'c': 0, 'd': 2, 'e': 4, 'f': 5, 'g': 7, 'a': 9, 'b': 11}


def compile_channel(text, sixteenth, drums=False):
    out = []
    octave, default_len = 4, 8
    current_len = None
    loop_at = None
    tie_next = False
    tokens = re.findall(r'[a-gr][+-]?\d*\.*|[kshoxt]\d*\.*|o\d|[<>&!]|l\d+|@\d+|\[|\]\d*|t\d+', text.replace(' ', '').replace('\n', '').replace('|', ''))

    def length_frames(tok):
        m = re.search(r'(\d*)(\.*)$', tok)
        n = int(m.group(1)) if m.group(1) else default_len
        frames = sixteenth * 16 // n
        add = frames
        for _ in m.group(2):
            add //= 2
            frames += add
        return frames

    def emit_len(frames):
        nonlocal current_len
        while frames > 64:
            emit_len_raw(64)
            out.append(0xC0)
            frames -= 64
        emit_len_raw(frames)

    def emit_len_raw(frames):
        nonlocal current_len
        if frames != current_len:
            out.append(0x80 | (frames - 1))
            current_len = frames

    def note(value, frames):
        # A long note becomes the note plus ties; '&' ties into this note.
        nonlocal tie_next
        first = min(frames, 64)
        emit_len_raw(first)
        out.append(0xC0 if tie_next else value)
        tie_next = False
        frames -= first
        while frames > 0:
            chunk = min(frames, 64)
            emit_len_raw(chunk)
            out.append(0xC0)
            frames -= chunk

    for tok in tokens:
        if tok == '!':
            loop_at = len(out)
            current_len = None
        elif tok == '>':
            octave += 1
        elif tok == '<':
            octave -= 1
        elif tok.startswith('o'):
            octave = int(tok[1:])
        elif tok.startswith('l'):
            default_len = int(tok[1:])
        elif tok.startswith('t'):
            pass
        elif tok.startswith('@'):
            out += [0xC1, int(tok[1:])]
        elif tok == '[':
            out += [0xC6, 0]
            current_len = None
        elif tok.startswith(']'):
            count = int(tok[1:] or 2)
            # patch the matching repeat start
            depth = 0
            for i in range(len(out) - 1, -1, -1):
                pass
            starts = [i for i, b in enumerate(out) if b == 0xC6 and i + 1 < len(out) and out[i + 1] == 0]
            out[starts[-1] + 1] = count
            out.append(0xC7)
            current_len = None
        elif tok == '&':
            tie_next = True
        elif drums and tok[0] in DRUM_IDS:
            note(DRUM_IDS[tok[0]], length_frames(tok))
        elif tok[0] == 'r':
            note(0, length_frames(tok))
        else:
            n = NOTES[tok[0]] + (1 if '+' in tok else -1 if '-' in tok else 0)
            note(12 * (octave + 1) + n, length_frames(tok))
    if loop_at is not None:
        out += [0xC3, loop_at & 255, loop_at >> 8]
    else:
        out.append(0xFF)
    return out


def note_freq():
    out = []
    for n in range(36, 108):
        f = 440 * 2 ** ((n - 69) / 12)
        x = round(2048 - 131072 / f)
        out.append(max(0, min(2047, x)))
    return out


from songs import SONGS, SFX  # noqa: E402


def build():
    h = '/* Generated by tools/music.py from tools/songs.py. */\n'
    h += 'static const uint16_t note_freq[72] = {' + ','.join(map(str, note_freq())) + '};\n'
    h += 'static const uint8_t waves[][16] = {' + ','.join('{' + ','.join(map(str, w)) + '}' for w in WAVES) + '};\n'
    ins = []
    for kind, a, env, gate, vol in INSTRUMENTS:
        ins += [a if kind == 'sq' else 0x80 | a, env, gate, vol]
    h += 'static const uint8_t instruments[] = {' + ','.join(map(str, ins)) + '};\n'
    kit = [0, 0]
    for k in 'kshoxt':
        kit += list(DRUMS[k])
    h += 'static const uint8_t drum_kit[] = {' + ','.join(map(str, kit)) + '};\n'
    table = []
    total = 0
    for name, song in SONGS:
        six = song['t']
        ptrs = []
        for ch, key in enumerate(('sq1', 'sq2', 'wave', 'noise')):
            if key in song:
                data = compile_channel(song[key], six, drums=(key == 'noise'))
                total += len(data)
                h += f'static const uint8_t song_{name}_{ch}[] = {{' + ','.join(map(str, data)) + '};\n'
                ptrs.append(f'song_{name}_{ch}')
            else:
                ptrs.append('0')
        table.append('{' + ','.join(ptrs) + '}')
    h += 'static const uint8_t * const songs[][4] = {{0,0,0,0},' + ','.join(table) + '};\n'
    sfx_ptrs, sfx_meta = ['0'], [0, 0]
    for name, ch, prio, steps in SFX:
        data = []
        for step in steps:
            data += list(step)
        data.append(0)
        h += f'static const uint8_t sfx_{name}[] = {{' + ','.join(map(str, data)) + '};\n'
        sfx_ptrs.append(f'sfx_{name}')
        sfx_meta += [ch, prio]
    h += 'static const uint8_t * const sfx_data[] = {' + ','.join(sfx_ptrs) + '};\n'
    h += 'static const uint8_t sfx_meta[] = {' + ','.join(map(str, sfx_meta)) + '};\n'
    (ROOT / 'src/music_data.h').write_text(h)
    print('music bytes', total)




def stream_frames(data):
    """Frames from the loop point to the end of one pass (expanding repeats)."""
    pos, frames, length, loop_frames = 0, 0, 12, None
    stack = []
    loop_target = None
    if data[-3] == 0xC3:
        loop_target = data[-2] | data[-1] << 8
    while pos < len(data):
        if pos == loop_target and loop_frames is None:
            loop_frames = frames
        b = data[pos]
        pos += 1
        if b < 0x80:
            frames += length
        elif b < 0xC0:
            length = (b & 0x3F) + 1
        elif b == 0xC0:
            frames += length
        elif b == 0xC1:
            pos += 1
        elif b == 0xC3:
            break
        elif b == 0xC6:
            stack.append([data[pos], pos + 1])
            pos += 1
        elif b == 0xC7:
            stack[-1][0] -= 1
            if stack[-1][0]:
                pos = stack[-1][1]
            else:
                stack.pop()
        elif b == 0xFF:
            break
    return frames - (loop_frames or 0), frames


def check():
    for name, song in SONGS:
        lengths = {}
        for ch, key in enumerate(('sq1', 'sq2', 'wave', 'noise')):
            if key in song:
                lengths[key] = stream_frames(compile_channel(song[key], song['t'], drums=(key == 'noise')))
        loops = {k: v[0] for k, v in lengths.items()}
        totals = {k: v[1] for k, v in lengths.items()}
        print(f'{name:10s} loop', loops, 'total', totals)
        assert len(set(loops.values())) == 1, (name, 'channels drift apart', loops)


if __name__ == '__main__':
    check()
    build()
