from pathlib import Path
import sys
exec((Path(__file__).resolve().parents[2]/'shared/assets.py').read_text().split('slug,out=sys.argv[1:]')[0])
tiles=[encode(canvas(8,8))]
for c in range(32,91):
 p=canvas(8,8);text(p,chr(c),1,0,1);tiles.append(encode(p))
for n,name in enumerate(['NEON','MOON','ECHO','BLOOM','ORBIT','HELLO','SWARM']):
 p=canvas(48,24)
 for y in range(24):
  for x in range(48):
   if 1<=x<47 and 1<=y<23:p[y][x]=1
   if x in (1,46) or y in (1,22):p[y][x]=2
 for x in range(17,31):p[0][x]=0;p[1][x]=0;p[2][x]=2
 # A tiny genre illustration in each cartridge's label recess.
 for y in range(4,14):
  for x in range(5,43):p[y][x]=0
 if n==0:
  for y in range(4,14):
   for x in (16-y//2,31+y//2,23):p[y][x]=3 if x!=23 or y%3 else 0
 elif n in (1,4):
  for y in range(4,14):
   for x in range(18,30):
    d=(x-23)**2+(y-8)**2
    if (d<20 if n==1 else 12<d<25):p[y][x]=3
 elif n==2:
  for y in range(5,13):
   for x in range(12,36):
    if y in (5,12) or x in (12,35) or (x%6==0 and y<10):p[y][x]=2
 elif n==3:
  for y in range(4,14):
   for x in range(15,33):
    if x in (20,27) or y in (6,11):p[y][x]=3
 elif n==5:
  for y in range(4,14):
   for x in range(18,30):
    if (x-23)**2+(y-8)**2<23:p[y][x]=3
  p[7][21]=p[7][25]=0
 else:
  for cx,cy in [(10,8),(16,10),(22,10),(28,8),(34,6)]:
   for y in range(cy-2,cy+3):
    for x in range(cx-2,cx+3):
     if (x-cx)**2+(y-cy)**2<6:p[y][x]=3
 text(p,name,(48-len(name)*6)//2,15,1,3)
 for y in range(0,24,8):
  for x in range(0,48,8):tiles.append(encode([r[x:x+8] for r in p[y:y+8]]))
assert len(tiles)<=256
Path(sys.argv[1]).write_text('#include <stdint.h>\n#define MENU_TILES %d\n'%len(tiles)+array('menu_tiles',sum(tiles,[])))
