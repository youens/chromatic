"""Exercise real cartridges with PyBoy joypad input, including complete puzzle solutions."""
from pathlib import Path
from collections import deque
import hashlib,json,re,struct
from pyboy import PyBoy
import bots
ROOT=Path(__file__).resolve().parents[1]
GAMES=json.loads((ROOT/'tools/games.json').read_text())
RUNTIME=[g for g in GAMES if g['slug'] not in ('hello-dot','dot-swarm','dreambase-invaders')]
class Console:
 def __init__(self,slug):
  self.slug=slug;self.root=ROOT/'games'/slug
  rom=(self.root/'dist'/f'{slug}.gbc').read_bytes()
  assert len(rom)==32768 and rom[0x143]==0xC0 and rom[0x147:0x14a]==bytes(3)
  check=0
  for v in rom[0x134:0x14d]:check=(check-v-1)&255
  assert rom[0x14d]==check
  assert (sum(rom)-sum(rom[0x14e:0x150]))&65535==int.from_bytes(rom[0x14e:0x150],'big')
  self.symbols={k:int(v,16) for k,v in re.findall(r'DEF (_\w+) 0x([\da-fA-F]+)',(self.root/'build'/f'{slug}.noi').read_text())}
  self.p=PyBoy(str(self.root/'dist'/f'{slug}.gbc'),window='null',sound_emulated=False);self.p.set_emulation_speed(0);self.held=set();self.p.tick(180)
 def get(self,n,i=0):return self.p.memory[self.symbols['_'+n]+i]
 def word(self,n,signed=False):
  v=self.get(n)+256*self.get(n,1);return v-65536 if signed and v>=32768 else v
 def set_keys(self,buttons):
  buttons=set(buttons)
  for k in self.held-buttons:self.p.button_release(k)
  for k in buttons-self.held:self.p.button_press(k)
  self.held=buttons
 def run(self,frames,buttons=()):self.set_keys(buttons);self.p.tick(frames)
 def press(self,key):self.run(6,[key]);self.run(16)
 def tap(self,key):self.run(4,[key]);self.run(4)
 def start(self):
  assert self.get('phase')==0
  self.press('b');assert self.get('phase')==1
  self.press('start');self.run(60);assert self.get('phase')==2
 def shot(self,name):self.p.screen.image.save(self.root/'art'/f'{name}.png')
 def close(self):self.p.stop(save=False)
results=[]
for info in RUNTIME:
 c=Console(info['slug'])
 try:
  c.shot('title');c.start()
  before=c.word('ticks');c.run(120);advance=c.word('ticks')-before
  assert advance==60,(c.slug,'update rate',advance)
  c.press('start');assert c.get('phase')==3
  before=c.word('ticks');c.run(60);assert c.word('ticks')==before
  c.press('start');assert c.get('phase')==2
  c.shot('gameplay')
  results.append({'game':c.slug,'cartridge':'valid','updates_per_120_frames':advance,'pause':'pass'})
  print('PASS boot, header, 30 Hz logic, help, pause:',c.slug,flush=True)
 finally:c.close()

# Solve every generated garden by rotating tiles through the actual controls.
c=Console('bloom-circuit')
try:
 c.start();initial=c.get('board');c.press('a');assert c.get('board')!=initial
 c.press('b');assert c.get('board')==initial,'Undo failed'
 for stage in range(1,6):
  assert c.get('stage')==stage
  width=c.get('width');count=c.get('count')
  for n in range(count):
   tx,ty=n%width,n//width
   while c.get('cx')<tx:c.press('right')
   while c.get('cx')>tx:c.press('left')
   while c.get('cy')<ty:c.press('down')
   while c.get('cy')>ty:c.press('up')
   for turns in range(4):
    if c.get('board',n)==c.get('solution',n):break
    c.press('a')
   assert c.get('board',n)==c.get('solution',n)
  c.shot('solved');c.run(120)
 assert c.get('phase')==4 and c.get('won')==1
 c.shot('results');c.press('a');assert c.get('phase')==2
 print('PASS five solvable gardens, undo, victory, replay',flush=True)
 results[3]['full_campaign']='five gardens solved through joypad'
finally:c.close()

# Pilot the orbital game by choosing a note and timing the strike window.
c=Console('orbit-choir')
try:
 c.start()
 for frame in range(4400):
  if c.get('phase')==4:break
  active=[(c.get('radius',i),c.get('nlane',i)) for i in range(6) if c.get('active',i)]
  keys=[]
  if active:
   radius,target=min(active);delta=(target-c.get('lane'))%8
   if delta:keys.append('right' if delta<=4 else 'left')
   elif 40<=radius<=51 and not c.get('hit_wait'):keys.append('a')
  if c.get('charge')==8:keys.append('b')
  c.run(1,keys)
  if frame==1000:c.shot('gameplay')
 assert c.get('phase')==4 and c.get('won')==1,(c.get('health'),c.word('ticks'))
 assert c.word('hits')>20
 c.shot('results');results[4]['full_round']={'hits':c.word('hits'),'score':c.word('score')}
 print('PASS orbital timing, combos, nova, full round:',results[4]['full_round'],flush=True)
finally:c.close()

# Take the lunar route by steering, braking and flipping gravity.
c=Console('moonthread')
try:
 c.start();flipped=False
 for frame in range(7600):
  if c.get('phase')==4:break
  stars=c.get('stars');x=c.word('px',True)/16+8;y=c.word('py',True)/16+8
  if stars!=7:
   i=next(i for i in range(3) if not stars&(1<<i));tx=[31,79,134][i];ty=42 if (i+c.get('stage'))&1 else 102
  else:tx,ty=146,40
  keys=[];dx=tx-x
  if abs(dx)>2:keys.append('right' if dx>0 else 'left')
  if abs(dx)<9:keys.append('b')
  gravity=c.get('gravity');gravity=gravity-256 if gravity>127 else gravity
  if abs(dx)<8 and ((ty<y-8 and gravity>0) or (ty>y+8 and gravity<0)):
   keys.append('a');flipped=True
  c.run(1,keys)
  if frame==400:c.shot('gameplay')
 assert flipped and c.get('phase')==4
 c.shot('results');results[1]['campaign']={'won':bool(c.get('won')),'room':c.get('stage'),'score':c.word('score'),'hearts':c.get('health'),'ticks':c.word('ticks'),'stars':c.get('stars')}
 assert c.get('won'),results[1]['campaign']
 print('PASS lunar gravity, stars, six chambers:',results[1]['campaign'],flush=True)
finally:c.close()

# Select safe lanes while traffic approaches.
c=Console('neon-wake')
try:
 c.start()
 for frame in range(4300):
  if c.get('phase')==4:break
  x=c.word('steering',True);threat=[]
  for i in range(6):
   z=c.get('cars',i*4)+256*c.get('cars',i*4+1);z=z-65536 if z>=32768 else z
   lane=c.get('cars',i*4+2);lane=lane-256 if lane>127 else lane
   if 45<z<106 and not c.get('cars',i*4+3):threat.append(lane*31)
  targets=[-48,0,48]
  safe=[p for p in targets if all(abs(p-t)>=17 for t in threat)]
  target=min(safe or targets,key=lambda p:abs(p-x))
  keys=[]
  if target-x>2:keys.append('right')
  elif target-x< -2:keys.append('left')
  if not threat and c.get('fuel')>30:keys.append('a')
  c.run(1,keys)
  if frame==600:c.shot('gameplay')
 assert c.get('phase')==4 and c.get('won')==1,(c.get('health'),c.word('ticks'))
 c.shot('results');results[0]['full_round']={'won':True,'score':c.word('score')}
 print('PASS traffic, boost, full racing round:',results[0]['full_round'],flush=True)
finally:c.close()

# Find routes through the real maze. The test can inspect hidden tiles; play cannot.
c=Console('echo-vault')
try:
 c.start();initial=c.get('energy');c.press('a');assert c.get('energy')<initial
 c.press('b');assert c.get('decoy')>0
 for step in range(810):
  if c.get('phase')==4:break
  x,y=c.get('px'),c.get('py');start=y*19+x;bits=c.get('collected')
  goals=[(7,3),(15,7),(3,11)]
  goal=next((g for i,g in enumerate(goals) if not bits&(1<<i)),(17,11));target=goal[1]*19+goal[0]
  guards={c.get('gy',i)*19+c.get('gx',i) for i in range(2)}
  route=None
  for avoid in (True,False):
   q=deque([start]);prev={start:None}
   while q:
    n=q.popleft()
    if n==target:break
    for nxt in (n-19,n+1,n+19,n-1):
     if 0<=nxt<247 and nxt not in prev and not c.get('maze',nxt) and not(avoid and nxt in guards):prev[nxt]=n;q.append(nxt)
   if target in prev:
    route=target
    while prev[route]!=start and prev[route] is not None:route=prev[route]
    break
  assert route is not None
  delta=route-start
  if delta==0:c.press('a')
  else:c.press({-19:'up',1:'right',19:'down',-1:'left'}[delta])
  if step==20:c.shot('gameplay')
 assert c.get('phase')==4
 c.shot('results');results[2]['campaign']={'won':bool(c.get('won')),'vault':c.get('stage'),'turns':c.word('turns'),'hearts':c.get('health')}
 assert c.get('won'),results[2]['campaign']
 print('PASS sonar, decoy, traversal, four vaults:',results[2]['campaign'],flush=True)
finally:c.close()
# Fly the whole storm. Screen captures eight frames apart show that each
# parallax band scrolls at its own rate while the HUD rows stay fixed.
def shift(a,b,rows):
 best=None
 for s in range(0,17):
  err=sum(int(abs(int(a[y][x+s][0])-int(b[y][x][0]))>8) for y in rows for x in range(0,140,2))
  if best is None or err<best[0]:best=(err,s)
 return best[1]
c=Console('stormkite')
try:
 c.start();c.run(240,['a'])
 a=c.p.screen.ndarray.copy();c.run(8,['a']);b=c.p.screen.ndarray.copy()
 bands={'hud':shift(a,b,range(137,143)),'far':shift(a,b,range(44,54)),'city':shift(a,b,range(64,94)),'rooftops':shift(a,b,range(100,118))}
 assert bands['hud']==0 and 0<bands['far']<bands['city']<bands['rooftops'],bands
 frames,before=c.p.frame_count,c.word('ticks')
 bots.stormkite(c,snapshot=lambda:c.shot('gameplay'))
 rate=(c.word('ticks')-before)/((c.p.frame_count-frames)/2)
 assert rate>0.98,('stormkite update rate',rate)
 assert c.get('phase')==4 and c.get('won')==1,(c.get('health'),c.word('ticks'),c.get('boss_hp'))
 c.shot('results')
 by_slug={r['game']:r for r in results}
 by_slug['stormkite']['parallax_shift_8_frames']=bands
 by_slug['stormkite']['full_flight']={'won':True,'score':c.word('score'),'hearts':c.get('health'),'kills':c.word('kills'),'ticks':c.word('ticks')}
 print('PASS raster parallax, four waves, Thunderhead:',bands,by_slug['stormkite']['full_flight'],flush=True)
finally:c.close()

# Play all nine holes. The planner uses the Python twin of the cartridge
# physics; every resting position must match it exactly.
c=Console('comet-links')
try:
 c.start();c.run(60);c.tap('b');assert c.get('scope_on')==1
 before=c.word('ticks');c.run(120,['right']);assert c.word('ticks')-before==60,('scope update rate',c.word('ticks')-before)
 outcome=bots.comet_links(c,snapshot=lambda:c.shot('gameplay'))
 assert c.get('phase')==4 and c.get('won')==1 and len(outcome['strokes'])==9,outcome
 assert outcome['physics_mismatches']==0,outcome
 c.shot('results');outcome['score']=c.word('score')
 {r['game']:r for r in results}['comet-links']['full_course']=outcome
 print('PASS nine holes, scope preview, exact physics parity:',outcome,flush=True)
finally:c.close()

# Clear all five wells, including the four stone-breaking wells.
c=Console('prism-well')
try:
 c.start();frames,before=c.p.frame_count,c.word('ticks')
 placed=bots.prism_well(c,snapshot=lambda:c.shot('gameplay'))
 rate=(c.word('ticks')-before)/((c.p.frame_count-frames)/2)
 assert rate>0.98,('prism update rate',rate)
 assert c.get('phase')==4 and c.get('won')==1,(c.get('well'),c.get('stones'),placed)
 c.shot('results')
 {r['game']:r for r in results}['prism-well']['campaign']={'won':True,'wells':5,'pieces':placed,'lights':c.word('cleared'),'best_chain':c.get('best_chain'),'score':c.word('score')}
 print('PASS five wells, stones, prisms, cascades:',placed,'pieces',flush=True)
finally:c.close()
(ROOT/'docs/collection-validation.json').write_text(json.dumps(results,indent=2)+'\n')
