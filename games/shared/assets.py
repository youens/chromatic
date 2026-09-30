"""Editable native pixel art for the Chromatic anthology."""
from pathlib import Path
import sys, math, random
import worlds
FONT = {
 'A':['01110','10001','10001','11111','10001','10001','10001'],
 'B':['11110','10001','10001','11110','10001','10001','11110'],
 'C':['01111','10000','10000','10000','10000','10000','01111'],
 'D':['11110','10001','10001','10001','10001','10001','11110'],
 'E':['11111','10000','10000','11110','10000','10000','11111'],
 'F':['11111','10000','10000','11110','10000','10000','10000'],
 'G':['01111','10000','10000','10111','10001','10001','01110'],
 'H':['10001','10001','10001','11111','10001','10001','10001'],
 'I':['11111','00100','00100','00100','00100','00100','11111'],
 'J':['00111','00010','00010','00010','10010','10010','01100'],
 'K':['10001','10010','10100','11000','10100','10010','10001'],
 'L':['10000','10000','10000','10000','10000','10000','11111'],
 'M':['10001','11011','10101','10101','10001','10001','10001'],
 'N':['10001','11001','11001','10101','10011','10011','10001'],
 'O':['01110','10001','10001','10001','10001','10001','01110'],
 'P':['11110','10001','10001','11110','10000','10000','10000'],
 'Q':['01110','10001','10001','10001','10101','10010','01101'],
 'R':['11110','10001','10001','11110','10100','10010','10001'],
 'S':['01111','10000','10000','01110','00001','00001','11110'],
 'T':['11111','00100','00100','00100','00100','00100','00100'],
 'U':['10001','10001','10001','10001','10001','10001','01110'],
 'V':['10001','10001','10001','10001','10001','01010','00100'],
 'W':['10001','10001','10001','10101','10101','10101','01010'],
 'X':['10001','10001','01010','00100','01010','10001','10001'],
 'Y':['10001','10001','01010','00100','00100','00100','00100'],
 'Z':['11111','00001','00010','00100','01000','10000','11111'],
 '0':['01110','10001','10011','10101','11001','10001','01110'],
 '1':['00100','01100','00100','00100','00100','00100','01110'],
 '2':['01110','10001','00001','00010','00100','01000','11111'],
 '3':['11110','00001','00001','01110','00001','00001','11110'],
 '4':['00010','00110','01010','10010','11111','00010','00010'],
 '5':['11111','10000','10000','11110','00001','00001','11110'],
 '6':['01110','10000','10000','11110','10001','10001','01110'],
 '7':['11111','00001','00010','00100','01000','01000','01000'],
 '8':['01110','10001','10001','01110','10001','10001','01110'],
 '9':['01110','10001','10001','01111','00001','00001','01110'],
 '!':['00100','00100','00100','00100','00100','00000','00100'],
 '.':['00000','00000','00000','00000','00000','00110','00110'],
 ':':['00000','00100','00100','00000','00100','00100','00000'],
 '+':['00000','00100','00100','11111','00100','00100','00000'],
 '-':['00000','00000','00000','11111','00000','00000','00000'],
 '/':['00001','00001','00010','00100','01000','10000','10000'],
 '=':['00000','00000','11111','00000','11111','00000','00000'],
 '>':['10000','01000','00100','00010','00100','01000','10000'],
 '<':['00001','00010','00100','01000','00100','00010','00001'],
}

def canvas(w,h): return [[0]*w for _ in range(h)]
def encode(p):
    out=[]
    for row in p:
        lo=hi=0
        for v in row: lo=(lo<<1)|(v&1);hi=(hi<<1)|((v>>1)&1)
        out.extend((lo,hi))
    return out

def text(p,s,x,y,scale,color=3):
    for c in s:
        for ry,row in enumerate(FONT.get(c,['00000']*7)):
            for rx,v in enumerate(row):
                if v=='1':
                    for yy in range(scale):
                        for xx in range(scale): p[y+ry*scale+yy][x+rx*scale+xx]=color
        x+=6*scale

def array(name, values):
    return 'const uint8_t '+name+'[] = {\n'+''.join('    '+','.join(str(v) for v in values[i:i+20])+',\n' for i in range(0,len(values),20))+'};\n'

slug,out=sys.argv[1:]
ids={'neon-wake':0,'moonthread':1,'echo-vault':2,'bloom-circuit':3,'orbit-choir':4,'stormkite':5,'comet-links':6,'prism-well':7}
kind=ids[slug]
bg=[encode(canvas(8,8))]
for c in range(32,91):
 p=canvas(8,8);text(p,chr(c),1,0,1);bg.append(encode(p))
patterns=[
['00000000','00000000','00000000','00010000','00000000','00000000','00000000','00000000'],
['33333333','21111112','21011012','21111112','21111112','21011012','21111112','22222222'],
['33333333','22222222','11111111','00000000','00000000','00000000','00000000','00000000'],
['00000000','00000000','00030000','00333000','00030000','00000000','00000000','00000000'],
['00000000','00030000','00232000','00232000','02333200','02333200','23333320','11111111'],
['03333330','32000023','32033023','32033023','32033023','32033023','32000023','03333330'],
['00000000','00033300','00322300','03223000','03320000','00200000','02000000','00000000'],
['00333300','03222230','32233223','32322223','32222223','32233223','03222230','00333300'],
['10101010','01010101','10101010','01010101','10101010','01010101','10101010','01010101'],
['00000032','00000321','00003211','00032111','00321111','03211111','32111111','21111111'],
['23000000','12300000','11230000','11123000','11112300','11111230','11111123','11111112'],
['00033000','00033000','00033000','00033000','00033000','00033000','00033000','00033000'],
['00000000','01110000','10001110','00000001','00000000','00111000','11000111','00000000'],
['33333333','11112111','11112111','22222222','12111111','12111111','12111111','22222222'],
['00000000','03303300','33333330','33333330','03333300','00333000','00030000','00000000'],
['11111111','10000001','10000001','10000001','10000001','10000001','10000001','11111111'],
]
for rows in patterns:bg.append(encode([[int(c) for c in row] for row in rows]))
while len(bg)<96:bg.append(encode(canvas(8,8)))
for mask in range(16):
 p=canvas(16,16)
 for y in range(16):
  for x in range(16):
   if x in (0,15) or y in (0,15):p[y][x]=1
   if 5<=x<=10 and 5<=y<=10:p[y][x]=2
   if ((mask&1 and y<=8 and 6<=x<=9) or (mask&2 and x>=8 and 6<=y<=9) or (mask&4 and y>=8 and 6<=x<=9) or (mask&8 and x<=8 and 6<=y<=9)):p[y][x]=3 if (x+y)%4 else 2
 p[7][7]=p[7][8]=p[8][7]=p[8][8]=3
 for yy,xx in [(0,0),(0,8),(8,0),(8,8)]:bg.append(encode([r[xx:xx+8] for r in p[yy:yy+8]]))
planet=canvas(32,32)
for y in range(32):
 for x in range(32):
  d=(x-15.5)**2+(y-15.5)**2
  if d<235:planet[y][x]=1 if x+y>40 else (2 if ((x*3+y*7)//9)%3 else 3)
for yy in range(0,32,8):
 for xx in range(0,32,8):bg.append(encode([r[xx:xx+8] for r in planet[yy:yy+8]]))
while len(bg)<192:bg.append(encode(canvas(8,8)))
if kind>=5:world=worlds.build(kind,bg)
# Each cartridge has its own 16-pixel hero silhouette.
hero=canvas(16,16)
if kind==0:
 for y in range(2,15):
  for x in range(1,15):
   if abs(x-7.5)<(4 if y<6 else 7):hero[y][x]=2
 for y in range(4,8):
  for x in range(4,12):hero[y][x]=1 if y!=4 else 3
 for x in range(2,14):hero[11][x]=3;hero[14][x]=1
 for x in (2,3,12,13):hero[12][x]=3
elif kind in (1,2):
 for y in range(1,11):
  for x in range(2,14):
   if (x-7.5)**2+(y-5.5)**2<32:hero[y][x]=3 if kind==1 else 2
 for y in range(3,8):
  for x in range(5,13):hero[y][x]=2 if kind==1 else 3
 for y in range(10,14):
  for x in range(4,12):hero[y][x]=3 if kind==1 else 1
 for x in (3,4,5,10,11,12):hero[14][x]=hero[15][x]=2
 hero[4][10]=3;hero[4][11]=3
elif kind==3:
 for y in range(2,14):
  for x in range(2,14):
   if (x-7.5)**2+(y-7.5)**2<30:hero[y][x]=2 if ((x+y)//3)%2 else 3
 for y in range(6,10):
  for x in range(6,10):hero[y][x]=1
elif kind==4:
 for y in range(1,15):
  for x in range(1,15):
   if abs(x-7.5)<(y/2):hero[y][x]=3 if x+y<18 else 2
 for y in range(7,12):hero[y][7]=hero[y][8]=1
 hero[15][4]=hero[15][11]=2
spr=[]
def add16(p):
 for yy,xx in [(0,0),(0,8),(8,0),(8,8)]:spr.extend(encode([r[xx:xx+8] for r in p[yy:yy+8]]))
if kind>=5:hero=world['hero']
add16(hero)
foe=canvas(16,16)
for y in range(2,14):
 for x in range(2,14):
  if (x-7.5)**2+(y-7.5)**2<40:foe[y][x]=2
for y in range(6,10):
 for x in range(3,13):foe[y][x]=3 if x in (4,5,10,11) else 1
if kind==0:
 foe=[r[:] for r in hero]
if kind>=5:foe=world['foe']
add16(foe)
small=[
['00030000','00030000','00333000','33333330','00333000','00030000','00030000','00000000'],
['00033000','03322330','03211230','32133123','32133123','03211230','03322330','00033000'],
['00333000','03223000','00333000','00030000','00033300','00030000','00033000','00000000'],
['03333330','03000330','03000330','03000330','03003330','03000330','03333330','00000000'],
['00033000','00300300','03000030','30000003','30000003','03000030','00300300','00033000'],
['00003330','00003030','00003000','00003000','00333000','03333000','00330000','00000000'],
['00022000','00233200','02333320','23333332','02333320','00233200','00022000','00000000'],
['33330000','30000000','30000000','30000000','00000000','00000000','00000000','00000000'],
]
for rows in small:spr.extend(encode([[int(c) for c in r] for r in rows]))
if kind>=5:
 for t in world['sprites']:spr.extend(t)
name=slug.upper().replace('-',' ');lines=name.split(' ')
p=canvas(160,48)
if len(lines)==1:lines=[name,'']
for i,line in enumerate(lines):
 text(p,line,(160-len(line)*12+2)//2,2+i*23,2,3 if i==0 else 2)
title_tiles=[];title_map=[]
for y in range(0,48,8):
 for x in range(0,160,8):
  t=encode([r[x:x+8] for r in p[y:y+8]])
  if t not in title_tiles:title_tiles.append(t)
  title_map.append(192+title_tiles.index(t))
assert len(title_tiles)<=64,(slug,len(title_tiles))
result='#include <stdint.h>\n'+f'#define TITLE_COUNT {len(title_tiles)}\n#define SPRITE_COUNT {len(spr)//16}\n'
result+=array('bg_data',sum(bg,[]))+array('title_data',sum(title_tiles,[]))+array('title_map',title_map)+array('sprite_data',spr)
scene=[0]*1024;scene_colors=[0]*1024
for y in range(3,17):
 for x in range(20):
  if (x*17+y*13)%31==0:scene[y*32+x]=63;scene_colors[y*32+x]=4
if kind==0:
 for y in range(2,7):
  for x in range(20):
   n=y*32+x
   if y==2 and x>12:scene[n]=68;scene_colors[n]=7
   elif y>3 and (x*7)%5<y-3:scene[n]=73;scene_colors[n]=6
elif kind in (1,4):
 for y in range(4):
  for x in range(4):
   n=(y+(5 if kind==1 else 8))*32+x+8;scene[n]=160+y*4+x;scene_colors[n]=4 if kind==1 else 5
if kind>=5:scene,scene_colors=world['scene'],world['colors']
result+=array('scene_map',scene)+array('scene_colors',scene_colors)
if kind==0:
 road=[];road_colors=[]
 for curve in range(-4,5):
  for phase in range(2):
   for y in range(7,17):
    w=1+(y-7)*8//9;center=10+int(curve*(17-y)/12);left=center-w;right=center+w
    for x in range(20):
     if x<left or x>right:t=68 if (x+y+phase)&1 else 0
     else:t=71 if x in (center-2,center+2) and (y+phase)&1 else 0
     if x==left:t=69
     if x==right:t=70
     road.append(t)
    road.extend([0]*12)
  for y in range(7,17):
   w=1+(y-7)*8//9;center=10+int(curve*(17-y)/12);left=center-w;right=center+w
   for x in range(20):road_colors.append(2 if x in (left,right) else 6 if x<left or x>right else 5)
   road_colors.extend([0]*12)
 result+=array('road_maps',road)+array('road_colors',road_colors)
if kind>=5:result+=world.get('extra','')
Path(out).write_text(result)
