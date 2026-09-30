"""Boot the real ROM and exercise controls, timing, and a full solo round."""
from pathlib import Path
import re
from pyboy import PyBoy
from PIL import Image

root = Path(__file__).resolve().parents[1]
symbols = dict(re.findall(r'DEF (_\w+) 0x([\dA-Fa-f]+)', (root / 'build/hello-dot.noi').read_text()))
address = lambda name: int(symbols[name], 16)
p = PyBoy(str(root / 'build/hello-dot.gbc'), window='null', sound_emulated=False)
p.set_emulation_speed(0)
base = address('_game')
u8 = lambda offset: p.memory[base + offset]
u16 = lambda offset: u8(offset) + 256 * u8(offset + 1)
scene = lambda: p.memory[address('_scene')]
def press(button, frames=8):
    p.button_press(button);p.tick(frames);p.button_release(button);p.tick(4)
def shot(name):
    folder = root / 'docs/screenshots'
    folder.mkdir(parents=True, exist_ok=True)
    p.screen.image.resize((640,576), Image.Resampling.NEAREST).save(folder / f'{name}.png')
try:
    p.tick(120)
    assert scene() == 0
    shot('title')
    press('b'); assert scene() == 1
    press('start'); assert scene() == 2
    before = u16(6);p.tick(120)
    assert u16(6)-before == 120, 'Expected one game tick per display frame'
    press('start'); assert scene() == 3
    before = u16(6);p.tick(90);assert u16(6)==before, 'Pause advanced game time'
    press('start'); assert scene() == 2
    x=u16(0);press('right',5);assert u16(0)>x
    press('a'); assert u8(16)>0, 'A did not start the dash cooldown'
    # Chase the nearest spark using only the actual joypad. No state injection.
    held=set()
    maximum=0
    for frame in range(3700):
        if scene()!=2: break
        x,y=(u16(0)>>4)+8,(u16(2)>>4)+8
        sparks=[(u8(57+i*3),u8(58+i*3)) for i in range(4) if not u8(59+i*3)]
        desired=set()
        if sparks:
            tx,ty=min(sparks,key=lambda s:(s[0]-x)**2+(s[1]-y)**2)
            if tx-x>3:desired.add('right')
            elif tx-x < -3:desired.add('left')
            if ty-y>3:desired.add('down')
            elif ty-y < -3:desired.add('up')
        for button in held-desired:p.button_release(button)
        for button in desired-held:p.button_press(button)
        held=desired;p.tick(1)
        maximum=max(maximum,u8(19))
        if u8(21)==1 and u8(24)==10:shot('hello-burst')
        if frame==600:shot('gameplay')
    assert scene()==4, 'Round did not end'
    assert u8(22)>=5 and u8(21)>=1, 'Could not collect a burst through real controls'
    shot('results')
    print(f'PASS: real ROM boot, help, 60 fps logic, pause, movement, dash, round end; score={u16(8)}, sparks={u8(22)}, bursts={u8(21)}, max multiplier={maximum}, frames={u16(6)}')
    for button in held:p.button_release(button)
    press('a');assert scene()==2, 'Replay did not start'
    print('PASS: replay')
finally:
    p.stop(save=False)
