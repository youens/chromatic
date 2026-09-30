from pathlib import Path
import re,json,hashlib
from pyboy import PyBoy
root=Path(__file__).resolve().parents[1]; b=root/'build';rom=b/'dot-swarm.gbc'
sym={k:int(v,16) for k,v in re.findall(r'DEF (_\w+) 0x([\da-fA-F]+)',(b/'dot-swarm.noi').read_text())}
p=PyBoy(str(rom),window='null',sound_emulated=False);p.set_emulation_speed(0)
held=set();checks=[]
def run(n=2,keys=()):
 global held
 keys=set(keys)
 for k in held-keys:p.button_release(k)
 for k in keys-held:p.button_press(k)
 held=keys;p.tick(n)
def get(k,o=0):return p.memory[sym['_'+k]+o]
def put(k,v,o=0):p.memory[sym['_'+k]+o]=v
def shot(n):p.screen.image.save(b/(n+'.png'))
def press(k):run(20,[k]);run(20)
def steps(n,keys=()):
 start=get('step_clock');frames=0
 while ((get('step_clock')-start)&255)<n:
  run(1,keys);frames+=1
  assert frames<240, 'game update stalled'
 # Complete the current update before inspecting its result.
 run(20,keys)
def arena(length=10):
 for i in range(4):
  off=i*99
  put('snakes',1 if i==0 else 0,off+98)
  put('snakes',length if i==0 else 6,off+96)
  put('snakes',0,off+97)
  for j in range(48):put('snakes',20-min(j,10),off+j);put('snakes',16,off+48+j)
 for j in range(80):put('fx',2,j);put('fy',2,j)
 put('step_clock',0);put('boost_clock',0)
def until(predicate,keys=(),limit=240):
 for _ in range(limit):
  if predicate():return
  run(1,keys)
 raise AssertionError('condition timed out')
def reset_arena(length=10):
 run(2)
 put('phase',3);run(30)
 arena(length);put('ended',0);put('won',0);put('phase',2)
try:
 run(180);assert get('phase')==0;shot('title')
 press('b');assert get('phase')==1;shot('help');press('start')
 if get('phase')==4:press('a')
 reset_arena();put('fx',21);put('fy',16)
 until(lambda:get('snakes',96)==11);checks.append('eat and grow');shot('arena')
 reset_arena();until(lambda:get('snakes',96)<10,['a']);checks.append('boost consumes tail')
 reset_arena(6);run(20,['a']);assert get('snakes',96)==6 and get('boosting')==0;checks.append('minimum boost length')
 put('phase',3);run(30);x=get('snakes');run(60);assert get('snakes')==x;checks.append('pause')
 reset_arena()
 # Cross a deliberately folded section of our own trail.
 for j in range(1,10):put('snakes',21,j);put('snakes',16,48+j)
 until(lambda:get('snakes')>=22)
 assert get('phase')==2 and get('ended')==0
 checks.append('own trail crossing is safe')
 reset_arena();put('snakes',38);until(lambda:get('phase')==4);shot('results');checks.append('wall collision')
 press('a');assert get('phase')==2;checks.append('restart')
 reset_arena();put('snakes',1,99+98);put('snakes',6,99+96)
 for j in range(6):put('snakes',21,99+j);put('snakes',16+j,99+48+j)
 until(lambda:get('phase')==4);checks.append('rival body collision')
 reset_arena(47);put('fx',21);put('fy',16)
 until(lambda:get('phase')==4);assert get('won')==1;shot('win');checks.append('48 dot victory')
 reset_arena();put('snakes',1,99+98);put('snakes',6,99+96)
 put('snakes',10,99);put('snakes',10,99+48)
 for j,(x,y) in enumerate([(11,10),(11,11),(10,11),(9,11),(9,10),(9,9),(10,9),(11,9)],1):
  put('snakes',x,j);put('snakes',y,48+j)
 put('step_clock',2);until(lambda:get('kills')>0);checks.append('trapped rival elimination')
 reset_arena();start=get('snakes');run(30);moved=get('snakes')-start
 assert 2<=moved<=5,('speed',moved);checks.append('moderate movement speed')
 shot('play')
 data=rom.read_bytes();assert data[0x143]==0xc0
 assert data[0x14d]==(-sum(data[0x134:0x14d])-25)&255
 (b/'validation.json').write_text(json.dumps({'sha256':hashlib.sha256(data).hexdigest(),'passed':checks,'tiles_per_half_second':moved},indent=2)+'\n')
 print('PASS:',', '.join(checks), 'speed:',moved*2,'tiles/sec')
finally:p.stop(save=False)
