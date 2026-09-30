from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/"shared"))
exec((Path(__file__).resolve().parents[2]/'shared/assets.py').read_text().split('slug,out=sys.argv[1:]')[0])
bg=[encode(canvas(8,8))]
for c in range(32,91):
 p=canvas(8,8);text(p,chr(c),1,0,1);bg.append(encode(p))
while len(bg)<192:bg.append(encode(canvas(8,8)))
# Authored 2bpp neon tiles, with a dark halo and a bright inner face.
for idx,radius in [(60,2.4),(61,3.7),(62,3.9),(63,1.0),(66,3.0),(68,2.8),(69,3.7)]:
 p=canvas(8,8)
 for y in range(8):
  for x in range(8):
   d=(x-3.5)**2+(y-3.5)**2
   if d<radius*radius:p[y][x]=2 if d<(radius-1.1)**2 else 1
 p[2][3]=p[2][4]=3
 if idx==69:p[3][3]=p[3][4]=p[4][3]=p[4][4]=3
 bg[idx]=encode(p)
for idx in (64,65):
 p=canvas(8,8)
 for y in range(8):
  for x in range(8):p[y][x]=2 if (x+y+idx)%6<2 else 1
 bg[idx]=encode(p)
for idx in range(70,74):
 p=canvas(8,8)
 if (idx-70)&1:
  for y in range(8):p[y][0]=1
 if (idx-70)&2:
  for x in range(8):p[0][x]=1
 bg[idx]=encode(p)
for idx in (74,75):
 p=canvas(8,8)
 for y in range(8):
  for x in range(8):
   p[y][x]=1
   if idx==74 and x in (2,5) and y in (2,3,6):p[y][x]=3
   if idx==75 and y==0:p[y][x]=2
 bg[idx]=encode(p)
# Directional eyes keep the player distinguishable from its tail.
for d,(xx,yy) in enumerate([(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1),(0,-1),(1,-1)]):
 p=canvas(8,8)
 for y in range(8):
  for x in range(8):
   dist=(x-3.5)**2+(y-3.5)**2
   if dist<15:p[y][x]=2 if dist<8 else 1
 cx,cy=3+xx,3+yy
 if xx:
  p[max(1,cy-1)][cx]=3;p[min(6,cy+2)][cx]=3
 else:
  p[cy][2]=3;p[cy][5]=3
 bg[80+d]=encode(p)
p=canvas(160,48)
# Offset shadow and oversized lettering, constrained to the title tile bank.
for word,y in [('DOT',1),('SWARM',25)]:
 x=(160-len(word)*18)//2
 text(p,word,x+1,y+2,3,1)
 text(p,word,x,y,3,3)
tt=[];tm=[]
for y in range(0,48,8):
 for x in range(0,160,8):
  t=encode([r[x:x+8] for r in p[y:y+8]])
  if t not in tt:tt.append(t)
  tm.append(192+tt.index(t))
assert len(tt)<=64, len(tt)
spr=[]
for ty in range(2):
 for tx in range(2):
  p=canvas(8,8)
  for y in range(8):
   for x in range(8):
    d=(x+tx*8-7.5)**2+(y+ty*8-7.5)**2
    if d<49:p[y][x]=3 if d<25 else 2
  spr+=encode(p)
Path(sys.argv[1]).write_text('#include <stdint.h>\n#define TITLE_COUNT %d\n#define SPRITE_COUNT 4\n'%len(tt)+array('bg_data',sum(bg,[]))+array('title_data',sum(tt,[]))+array('title_map',tm)+array('sprite_data',spr)+array('scene_map',[0]*1024)+array('scene_colors',[0]*1024))
