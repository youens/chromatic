#!/usr/bin/env python3
"""Editable, exact-cell Dotwing artwork and Game Boy Color 2bpp export.

Generated concept references are retained in art/, but are never downsampled
into ROM assets. Every native sprite below is authored at its true resolution.
The attempted grid reference has 18 cells, so is unsuitable for exact recovery.
"""
from pathlib import Path
import ast
import math
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'art'
ART.mkdir(exist_ok=True)
SOURCE_FONT = ROOT.parent / 'hello-dot/tools/assets.py'
tree = ast.parse(SOURCE_FONT.read_text())
FONT = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'FONT' for t in n.targets))
FONT.update({'?':['01110','10001','00001','00010','00100','00000','00100'],
             '(':['00010','00100','01000','01000','01000','00100','00010'],
             ')':['01000','00100','00010','00010','00010','00100','01000'],
             '%':['11001','11010','00100','01000','10110','00110','00000']})
COLORS = ['#f392bd','#7eb9ff','#78e8be','#ffdb75','#bc9bff','#7deaf1']
INK, WHITE = '#172a48', '#fff4d4'
PALETTES = [[INK, INK, c, WHITE] for c in COLORS]
RIVALS = [[INK, INK, '#b4cce2', '#ffffff'],
          [INK, INK, '#fa8b72', '#ffe1be'],
          [INK, INK, '#7bafff', '#b6f8ff'],
          [INK, INK, '#c69bff', '#f4e6ff']]

def canvas(w, h, value=0): return [[value] * w for _ in range(h)]
def rows(*s):
    assert len({len(x) for x in s}) == 1
    return [[int(x) for x in row] for row in s]
def encode_tile(p):
    out = []
    for row in p:
        lo = hi = 0
        for v in row:
            lo = (lo << 1) | (v & 1)
            hi = (hi << 1) | ((v >> 1) & 1)
        out += [lo, hi]
    return out
def encode_sprite(p):
    return sum((encode_tile([r[x:x+8] for r in p[y:y+8]]) for x in range(0,len(p[0]),8) for y in range(0,len(p),8)), [])
def text(p, s, x, y, scale=1, color=3):
    for c in s.upper():
        for ry,row in enumerate(FONT.get(c,['00000']*7)):
            for rx,v in enumerate(row):
                if v == '1':
                    for dy in range(scale):
                        for dx in range(scale):
                            if 0 <= y+ry*scale+dy < len(p) and 0 <= x+rx*scale+dx < len(p[0]):
                                p[y+ry*scale+dy][x+rx*scale+dx] = color
        x += 6*scale
def rect(p, x, y, w, h, v):
    for yy in range(max(0,y),min(len(p),y+h)):
        for xx in range(max(0,x),min(len(p[0]),x+w)): p[yy][xx] = v
def line(p,x1,y1,x2,y2,v):
    n=max(abs(x2-x1),abs(y2-y1))
    for i in range(n+1):
        x=round(x1+(x2-x1)*i/max(1,n));y=round(y1+(y2-y1)*i/max(1,n))
        if 0<=y<len(p) and 0<=x<len(p[0]):p[y][x]=v

# Four explicit tiles arranged column-major for the 8x16 sprite mode.
PLANE=rows(
 '0000000220000000','0000002332000000','0000002332000000','0000022332200000',
 '0000022112200000','0000221331220000','0002221311222000','0023221331223200',
 '0233222112223320','2332222332222332','3322222332222233','0222322332232220',
 '0002312332132000','0002112332112000','0003300220033000','0000000220000000')
GROK=rows(
 '0000001111000000','0000113333110000','0001332222331000','0013220000223100',
 '0132200000322310','0132000003322310','1322000033222231','1322000332222231',
 '1322003322222231','1322033222222231','0132332222222310','0133322222222310',
 '0013222222223100','0001332222331000','0000113333110000','0000001111000000')
CLAUDE=rows(
 '0000010000100000','0000021001200000','0010023113200100','0001223333221000',
 '0122333333332210','0012333223332100','1223332112333221','0123321331233210',
 '0123321331233210','1223332112333221','0012333223332100','0122333333332210',
 '0001223333221000','0010023113200100','0000021001200000','0000010000100000')
GEMINI=rows(
 '0000000220000000','0000002332000000','0000002332000000','0000022332200000',
 '0000022332200000','0000222332220000','0022223333222200','0233333333333320',
 '0233333333333320','0022223333222200','0000222332220000','0000022332200000',
 '0000022332200000','0000002332000000','0000002332000000','0000000220000000')
MUSE=rows(
 '0000001111000000','0000112222110000','0001222332221000','0012333333332100',
 '0123322332233210','0123323333233210','1233323112333321','1233323112333321',
 '1233322112233321','1233322112233321','0123322112233210','0123322112233210',
 '0012333333332100','0001222332221000','0000112222110000','0000001111000000')

def emblem(p,kind,cx,cy,size):
    """Logo-inspired readable geometry, never a claim of official exact logos."""
    for y in range(cy-size,cy+size+1):
        for x in range(cx-size,cx+size+1):
            dx=x-cx;dy=y-cy;put=False
            if kind==0:put=(size-2)**2<=dx*dx+dy*dy<=size**2 or (abs(dx+dy)<2 and abs(dx)<=size-1)
            elif kind==1:put=(dx*dx+dy*dy<=6) or ((abs(dx)<=1 or abs(dy)<=1 or abs(dx-dy)<=1 or abs(dx+dy)<=1) and abs(dx)+abs(dy)<=size+2)
            elif kind==2:put=abs(dx)*abs(dy)<=size//2 and abs(dx)+abs(dy)<=size
            else:put=(abs(dx)==size-3 and abs(dy)<=size-3) or (dy==-size+3+abs(dx) and abs(dx)<=size-3)
            if put and 0<=y<len(p) and 0<=x<len(p[0]):p[y][x]=3

def boss(kind):
    p=canvas(32,32)
    # A broad aircraft carrier with separate wing tips, armoured central fuselage.
    for y in range(2,30):
        for x in range(2,30):
            dx=abs(x-15);dy=abs(y-15)
            if (dx<=9 and dy<=12) or (dy<=6 and dx<=14):p[y][x]=2 if dy<11 else 1
    for x in (2,27):rect(p,x,11,3,10,1);rect(p,x,12,2,4,3)
    for x in (6,23):rect(p,x,7,3,18,1);rect(p,x,20,2,4,3)
    rect(p,11,4,10,3,3);rect(p,12,25,8,3,1)
    # Hollow darker badge separates lab insignia from the airframe.
    for y in range(8,24):
        for x in range(8,24):
            if (x-15)**2+(y-15)**2 <= 58:p[y][x]=1
    emblem(p,kind,15,15,7)
    return p
BOSSES=[boss(k) for k in range(4)]

def small(s):
    p=rows(*s)
    while len(p)<16:p.append([0]*8)
    return p
SMALL=[small(s) for s in [
 ['00033000','00033000','00033000','00033000','00033000','00033000','00022000','00022000'],
 ['00000000','00022000','00233200','00233200','00022000','00000000'],
 ['00000000','00222200','02333320','23233232','23233232','02333320','00222200'],
 ['00000000','02222220','23333332','23222232','23323332','23223332','02222220'],
 ['00000000','03300330','33333333','33333333','03333330','00333300','00033000'],
 ['00033000','00033000','00333300','33333333','00333300','00033000','00033000'],
 ['03000300','00333000','23333320','33323333','23333320','00333000','03000300'],
]]

def dot_pixel(x,y,shape=0,face=0,gear=0):
    dx=abs(x-15);dy=abs(y-18)
    if shape==0:inside=dx*dx+dy*dy<=111
    elif shape==1:inside=dx<=10 and dy<=10 and (dx<9 or dy<9)
    elif shape==2:inside=8<=y<=29 and dx<=(y-7)//2
    elif shape==3:inside=(dx<=5 and dy<=10) or ((dx-5)**2+dy*dy<=100 and dx<=12)
    else:inside=((dx<=3 and dy<=11) or (dy<=3 and dx<=12) or (dx+dy<=11)) and not (dx>8 and dy>3)
    v=2 if inside else 0
    if inside and y>=27:v=1
    if inside and ((x==8 and y==16) or (x==9 and y==15)):v=3
    if inside:
        if face==1 and 16<=y<=18 and 9<=x<=22:v=1
        elif face==2:
            if (x in (11,12) and y in (16,17)) or (18<=x<=21 and y==17):v=1
        elif face==3:
            if (11<=x<=13 and y==15) or (18<=x<=20 and y==15) or (x in (12,19) and y==17):v=1
        elif (x in (11,12,19,20)) and y in (16,17):v=1
        if face!=3 and 14<=x<=17 and y==22:v=1
        if face==0 and x in (13,18) and y==21:v=1
    if gear==1:
        if 8<=x<=22 and 6<=y<=10:v=1
        if 8<=x<=24 and y==11:v=3
        if 13<=x<=17 and y==7:v=3
    elif gear==2:
        if 8<=x<=23 and y in (3,5):v=3
        if x in (7,24) and y==4:v=3
    elif gear==3:
        if 8<=x<=23 and y in (14,19):v=3
        if x in (8,14,17,23) and 14<=y<=19:v=3
        if 15<=x<=16 and y==16:v=3
    return v

def portrait(shape=0,face=0,gear=0):return [[dot_pixel(x,y,shape,face,gear) for x in range(32)] for y in range(32)]

bg=[canvas(8,8)]
for c in range(32,91):
    p=canvas(8,8);text(p,chr(c),1,0,color=1);bg.append(p)
assert len(bg)==60
SCENERY=[canvas(8,8) for _ in range(36)]
SCENERY[1][3]=[0,0,1,1,1,1,0,0]
SCENERY[2][2][3]=1;SCENERY[2][1][3]=1;SCENERY[2][2][2]=1;SCENERY[2][2][4]=1;SCENERY[2][3][3]=1
cloud=canvas(32,24)
for y in range(24):
    for x in range(32):
        # Contour of a bank of fluffy clouds, deliberately tile aligned.
        if ((x-8)**2+(y-10)**2<76) or ((x-18)**2+(y-7)**2<80) or ((x-26)**2+(y-12)**2<47):
            cloud[y][x]=3 if y<14 else 2
for ty in range(3):
    for tx in range(4):SCENERY[3+ty*4+tx]=[r[tx*8:tx*8+8] for r in cloud[ty*8:ty*8+8]]
for idx in range(15,23):
    p=SCENERY[idx]
    rect(p,0,0,8,8,1);rect(p,1,1,6,7,2)
    for y in (2,5):
        for x in (2,5):p[y][x]=3
    if idx%2:rect(p,0,6,8,2,1)
for idx in range(23,27):
    p=SCENERY[idx]
    for y in range(8):
        for x in range(8):
            if x==3 or y==(idx-23)*2:p[y][x]=2
    p[(idx-23)*2][3]=3
for idx in range(27,31):
    p=SCENERY[idx]
    for x in range(8):p[(x+idx)%8][x]=1
# 91..95: Decorative menu line, corners, glint, inset frame and hangar floor.
SCENERY[31]=rows('00000000','00000000','00000000','11111111','00000000','00000000','00000000','00000000')
SCENERY[32]=rows('00000000','01111111','01000000','01000000','01000000','01000000','01000000','01000000')
SCENERY[33]=rows('00010000','00010000','00111000','11111110','00111000','00010000','00010000','00000000')
SCENERY[34]=rows('00000000','01100110','11111111','11111111','01111110','00111100','00011000','00000000')
SCENERY[35]=rows('10000000','01000000','00100000','00010000','00001000','00000100','00000010','00000001')
bg+=SCENERY
logo=canvas(96,16)
text(logo,'DOTWING',6,1,2,3)
# Small trailing wing-shaped underlining motif using the cyan palette color.
for x in range(88,96):logo[14][x]=2
bg += [[r[x:x+8] for r in logo[y:y+8]] for y in (0,8) for x in range(0,96,8)]
while len(bg)<128:bg.append(canvas(8,8))

sprite_pixels=[PLANE,GROK,CLAUDE,GEMINI,MUSE,BOSSES[0]]+SMALL
SPRITES=sum((encode_sprite(p) for p in sprite_pixels),[])
assert len(SPRITES)==50*16
SWATCH=canvas(8,16)
for y in range(4,12):
 for x in range(8):
  if (x-3.5)**2+(y-7.5)**2<=15:SWATCH[y][x]=2

def arr(name,data):
 return 'static const uint8_t '+name+'[] = {\n'+''.join('  '+','.join(str(v) for v in data[i:i+24])+',\n' for i in range(0,len(data),24))+'};\n'
def cgb(hexcolor):
 r,g,b=tuple(int(hexcolor[i:i+2],16) for i in (1,3,5));return (r>>3)|((g>>3)<<5)|((b>>3)<<10)

runtime=r'''
/* Art scratch RAM overlays the inactive games' shared screen maps. */
#define DW_ART_BUF ((uint8_t *)0xD000)
#define DW_ART_ATT ((uint8_t *)0xD020)
static uint8_t dot_pixel(uint8_t x,uint8_t y,uint8_t scale) {
  uint8_t dx,dy,v=0,inside=0;
  if(scale) {
    x=(uint8_t)(x*2); y=(uint8_t)(y*2);
    /* Preserve the single-cell pupils when reducing the pilot to 16x16. */
    if(y==16)y=17;
    if(x==20)x=19;
  }
  dx=x>15?x-15:15-x;dy=y>18?y-18:18-y;
  switch(dw_profile.shape) {
    case 1:inside=dx<=10&&dy<=10&&(dx<9||dy<9);break;
    case 2:inside=y>=8&&y<=29&&dx<=(y-7)/2;break;
    case 3:inside=(dx<=5&&dy<=10)||(dx<=12&&(int16_t)(dx-5)*(int16_t)(dx-5)+(uint16_t)dy*dy<=100);break;
    case 4:inside=((dx<=3&&dy<=11)||(dy<=3&&dx<=12)||(dx+dy<=11))&&!(dx>8&&dy>3);break;
    default:inside=(uint16_t)dx*dx+(uint16_t)dy*dy<=111;break;
  }
  if(inside) {
    v=y>=27?1:2;
    if((x==8&&y==16)||(x==9&&y==15))v=3;
    if(dw_profile.face==1&&y>=16&&y<=18&&x>=9&&x<=22)v=1;
    else if(dw_profile.face==2) {
      if(((x==11||x==12)&&(y==16||y==17))||(x>=18&&x<=21&&y==17))v=1;
    } else if(dw_profile.face==3) {
      if((x>=11&&x<=13&&y==15)||(x>=18&&x<=20&&y==15)||((x==12||x==19)&&y==17))v=1;
    } else if((x==11||x==12||x==19||x==20)&&(y==16||y==17))v=1;
    if(dw_profile.face!=3&&x>=14&&x<=17&&y==22)v=1;
    if(!dw_profile.face&&(x==13||x==18)&&y==21)v=1;
  }
  if(dw_profile.gear==1) {
    if(x>=8&&x<=22&&y>=6&&y<=10)v=1;
    if(x>=8&&x<=24&&y==11)v=3;
    if(x>=13&&x<=17&&y==7)v=3;
  } else if(dw_profile.gear==2) {
    if(x>=8&&x<=23&&(y==3||y==5))v=3;
    if((x==7||x==24)&&y==4)v=3;
  } else if(dw_profile.gear==3) {
    if(x>=8&&x<=23&&(y==14||y==19))v=3;
    if((x==8||x==14||x==17||x==23)&&y>=14&&y<=19)v=3;
    if(x>=15&&x<=16&&y==16)v=3;
  }
  return v;
}
static void dot_tiles(uint8_t first,uint8_t size) {
  uint8_t tx,ty,x,y,v,n=0,scale=size==16;
  for(tx=0;tx<size;tx+=8)for(ty=0;ty<size;ty+=8) {
    for(y=0;y!=8;++y) {
      DW_ART_BUF[y*2]=0;DW_ART_BUF[y*2+1]=0;
      for(x=0;x!=8;++x) {
        v=dot_pixel(tx+x,ty+y,scale);
        DW_ART_BUF[y*2]=(DW_ART_BUF[y*2]<<1)|(v&1);
        DW_ART_BUF[y*2+1]=(DW_ART_BUF[y*2+1]<<1)|(v>>1);
      }
    }
    set_sprite_data(first+n,1,DW_ART_BUF);++n;
  }
}
static void plane_tiles(void) {
  uint8_t tx,ty,x,y,px,py,v,n=0;
  for(tx=0;tx!=16;tx+=8)for(ty=0;ty!=16;ty+=8) {
    for(y=0;y!=8;++y) {
      DW_ART_BUF[y*2]=0;DW_ART_BUF[y*2+1]=0;
      for(x=0;x!=8;++x) {
        px=tx+x;py=ty+y;v=plane_pixels[(uint16_t)py*16+px];
        if(px>=6&&px<=9&&py>=5&&py<=8) {
          v=dot_pixel((px-6)*6+6,(py-5)*5+8,0);
          if(!v)v=1;
          /* A two-pixel smile reads better than sparsely sampled large eyes. */
          if(py==7&&(px==6||px==9))v=1;
          if(py==7&&dw_profile.face==1)v=1;
          if(py==7&&dw_profile.face==2&&px==9)v=3;
          if(py==6&&dw_profile.face==3&&(px==6||px==9))v=1;
        }
        DW_ART_BUF[y*2]=(DW_ART_BUF[y*2]<<1)|(v&1);
        DW_ART_BUF[y*2+1]=(DW_ART_BUF[y*2+1]<<1)|(v>>1);
      }
    }
    set_sprite_data(n,1,DW_ART_BUF);++n;
  }
}
void dw_dot_art(void) BANKED {
  uint8_t i;
  VBK_REG=0;
  set_sprite_palette(0,1,dot_palettes+((uint16_t)dw_profile.color%6)*4);
  for(i=0;i!=6;++i)set_sprite_palette(i+1,1,dot_palettes+(uint16_t)i*4);
  dot_tiles(DW_SPR_PORTRAIT,32);dot_tiles(DW_SPR_PILOT,16);plane_tiles();
}
void dw_palettes(uint8_t sector) BANKED {
  if(!sector||sector>4)sector=1;
  set_bkg_palette(0,8,sky_palettes+(uint16_t)(sector-1)*32);
  set_sprite_palette(0,1,dot_palettes+((uint16_t)dw_profile.color%6)*4);
  set_sprite_palette(1,7,flight_palettes);
}
void dw_art_load(void) BANKED {
  VBK_REG=0;set_bkg_data(0,128,bg_tiles);set_sprite_data(0,50,sprite_tiles);
  set_sprite_data(DW_SPR_SWATCH,2,swatch_tiles);
  dw_palettes(1);dw_dot_art();
}
/* 32x32 seamless vertical map. Clouds have authored 4x3 tile contours. */
static uint8_t sky_tile(uint8_t x,uint8_t y,uint8_t sector) {
  uint8_t cx,cy;
  if(sector<3) {
    cy=y<6?3:(y<18?12:23);
    cx=y<6?2:(y<18?14:24);
    if(x>=cx&&x<cx+4&&y>=cy&&y<cy+3)return 63+(y-cy)*4+(x-cx);
    if(sector==2&&y>=27&&x<7)return 75+((x+y)&7);
  } else {
    if(x<4||x>=28)return 75+((y+x)&7);
    if((x==5||x==26)&&((y&7)<4))return 83+(y&3);
    if(sector==4&&((x+y)&15)==2)return 87+(y&3);
    if(x>=14&&x<18&&y>=8&&y<11)return 63+(y-8)*4+x-14;
  }
  if(((uint16_t)x*7+(uint16_t)y*11)%43==0)return 62;
  if(((x*3+y*5)&31)==4)return 61;
  return 60;
}
void dw_sky(uint8_t sector) BANKED {
  uint8_t x,y,t;
  if(!sector||sector>4)sector=1;
  dw_palettes(sector);
  set_sprite_data(DW_SPR_BOSS,16,boss_tiles+(uint16_t)(sector-1)*256);
  for(y=0;y!=32;++y) {
    for(x=0;x!=32;++x) {
      t=sky_tile(x,y,sector);DW_ART_BUF[x]=t;
      DW_ART_ATT[x]=(t>=75&&t<=82)?2:((t>=83)?3:1);
    }
    VBK_REG=0;set_bkg_tiles(0,y,32,1,DW_ART_BUF);
    VBK_REG=1;set_bkg_tiles(0,y,32,1,DW_ART_ATT);
  }
  VBK_REG=0;
}
'''

# Sector palette slots0and4..7 stay legible across backgrounds and HUDs.
sky=[]
for base,mid,cloudcolor,ground,light in [
 ('#17395a','#367091','#71b7ca','#25617d','#ade5d9'),
 ('#523855','#97618a','#d99b9d','#76425c','#ffc5a4'),
 ('#172944','#354a70','#6685a1','#285968','#80d4d9'),
 ('#181e40','#383466','#706798','#37336f','#b29dff')]:
 sky += [cgb(c) for c in [INK,WHITE,'#b9c9df','#ffffff',
                          base,mid,cloudcolor,WHITE,
                          base,ground,mid,light,
                          base,mid,light,WHITE,
                          INK,'#7deaf1','#78e8be',WHITE,
                          INK,'#91a4be','#6b819b',WHITE,
                          INK,'#ffdb75','#ffbe58',WHITE,
                          INK,WHITE,'#b7d7ef','#ffffff']]
dotp=[cgb(c) for p in PALETTES for c in p]
flight=[cgb(c) for p in RIVALS+[[INK,INK,'#7deaf1',WHITE],[INK,INK,'#ffce69',WHITE],[INK,INK,'#ffbd85','#ffffff']] for c in p]
def pal(name,v):return 'static const palette_color_t '+name+'[] = {'+','.join(hex(x) for x in v)+'};\n'
out='''/* Generated by tools/assets.py. Edit exact native cells in that file. */
#ifdef ARCADE
#pragma bank 29
#else
#pragma bank 4
#endif
#include "dotwing.h"
#include "art_ids.h"
'''
out+=arr('bg_tiles',sum((encode_tile(p) for p in bg),[]))
out+=arr('sprite_tiles',SPRITES)+arr('boss_tiles',sum((encode_sprite(p) for p in BOSSES),[]))
out+=arr('plane_pixels',sum(PLANE,[]))+arr('swatch_tiles',encode_sprite(SWATCH))
out+=pal('dot_palettes',dotp)+pal('flight_palettes',flight)+pal('sky_palettes',sky)+runtime
(ROOT/'src/art.c').write_text(out)

def rgba(p,palette):
 im=Image.new('RGBA',(len(p[0]),len(p)))
 for y,row in enumerate(p):
  for x,v in enumerate(row):
   if v:im.putpixel((x,y),tuple(int(palette[v][i:i+2],16) for i in (1,3,5))+(255,))
 return im
for name,p,palett in zip(['plane','grok','claude','gemini','muse'],[PLANE,GROK,CLAUDE,GEMINI,MUSE],[PALETTES[2]]+RIVALS):
 rgba(p,palett).save(ART/(name+'-native.png'))
for kind in range(4):rgba(BOSSES[kind],RIVALS[kind]).save(ART/f'boss-{kind+1}-native.png')

sheet=Image.new('RGB',(320,176),'#101c35')
for i,p in enumerate([PLANE,GROK,CLAUDE,GEMINI,MUSE]):
 pi=rgba(p,([PALETTES[2]]+RIVALS)[i]).resize((48,48),Image.Resampling.NEAREST)
 sheet.paste(pi,(12+i*62,10),pi)
for shape in range(5):
 pi=rgba(portrait(shape,shape%4,shape%4),PALETTES[shape]).resize((48,48),Image.Resampling.NEAREST)
 sheet.paste(pi,(12+shape*62,72),pi)
for i in range(4):
 pi=rgba(BOSSES[i],RIVALS[i]).resize((48,48),Image.Resampling.NEAREST)
 sheet.paste(pi,(12+i*80,125),pi)
sheet.save(ART/'native-contact-sheet.png')

# Promotional source: hand-authored pixels composed from the same native assets.
cover=Image.new('RGB',(400,210),'#183653');d=ImageDraw.Draw(cover)
for y in range(210):
 t=y/210;d.line((0,y,399,y),fill=(24+int(t*7),54+int(t*32),83+int(t*26)))
for i in range(36):
 x=(i*73+31)%400;y=(i*29+16)%210
 d.line((x,y,x+4,y),fill='#3b6681')
 if i%5==0:d.line((x+2,y-2,x+2,y+2),fill='#618ca3')
cloud_im=rgba(cloud,['#17395a','#367091','#497e99','#6fa6b7']).resize((96,72),Image.Resampling.NEAREST)
for x,y in [(195,-30),(329,15),(174,155),(342,153)]:cover.paste(cloud_im,(x,y),cloud_im)
# Soft city at horizon, kept clear of title typography.
for x in range(0,400,10):
 h=6+(x*13)%21;d.rectangle((x,210-h,x+8,209),fill='#244c65')
 for y in range(210-h+3,209,6):d.rectangle((x+3,y,x+4,y+1),fill='#4b8394')
for i,(kind,x,y) in enumerate([(0,284,25),(1,355,73),(2,207,121),(3,346,144)]):
 p=[GROK,CLAUDE,GEMINI,MUSE][kind];pi=rgba(p,RIVALS[kind]).resize((32,32),Image.Resampling.NEAREST)
 cover.paste(pi,(x,y),pi)
 d.line((x+15,y+36,x+15,y+42),fill=['#b4cce2','#fa8b72','#7bafff','#c69bff'][kind],width=2)
# Hero and cyan volleys.
hero=rgba(PLANE,PALETTES[2]).resize((80,80),Image.Resampling.NEAREST)
d.line((271,112,271,159),fill='#92edc6',width=3);d.line((296,112,296,167),fill='#92edc6',width=3)
cover.paste(hero,(245,72),hero)
for x,y in [(267,48),(297,42),(267,31),(297,21)]:
 d.rectangle((x,y,x+2,y+9),fill='#86f1ef');d.rectangle((x,y,x+2,y+2),fill='#fff4d4')
# Title and typography use the exact block font.
labels=canvas(400,210)
text(labels,'DOTWING',17,22,4,3);text(labels,'AI SKY SUPREMACY',19,58,1,2)
text(labels,'BUILD YOUR DOT.',19,90,1,3);text(labels,'TAKE THE SKY.',19,102,1,3)
text(labels,'TOKEN BOOSTS / LAB BOSSES',19,158,1,2)
text(labels,'GAME BOY COLOR',19,175,1,3)
im=rgba(labels,['#17395a','#367091','#78e8be',WHITE]);cover.paste(im,(0,0),im)
for i,c in enumerate(COLORS):
 pi=rgba(portrait(i%5,i%4,i%4),[INK,INK,c,WHITE]).resize((24,24),Image.Resampling.NEAREST)
 cover.paste(pi,(19+i*29,122),pi)
cover=cover.resize((1200,630),Image.Resampling.NEAREST)
cover.save(ART/'cover.png')
cover.resize((480,252),Image.Resampling.LANCZOS).save(ART/'cover-card-v1.webp',quality=90)
cover.resize((960,504),Image.Resampling.LANCZOS).save(ART/'cover-detail-v1.webp',quality=92)
cover.save(ART/'cover-share-v1.jpg',quality=95,subsampling=0,optimize=True)
(ART/'README.md').write_text('''# Dotwing native art

`../tools/assets.py` is the editable source of every ROM tile and promotional
pixel. Run `python3 games/dotwing/tools/assets.py` from the repository root to
rebuild `src/art.c`, native RGBA sources, the contact sheet and optimized covers.
Sprites are 2bpp, in 8x16 column-major layout; small actors16x16, bosses32x32.
The four lab motifs are playful interpretations, not exact official logos.

`plane-reference-v1.png` is an Image Generation silhouette reference. The
second `plane-grid-reference-v1.png` attempted an exact16x16 chart but actually
has18x18 cells, so recovery was rejected. Native cells were deliberately authored
in the generator, retaining the broad swept-wing silhouette and cockpit.
No generated reference was downsampled into a ROM sprite. The contact sheet is
a source-art preview; gameplay images are actual emulator captures when present.
''')
print(f'Wrote {ROOT / "src/art.c"}; bg128tiles, sprites50+swatch2, bosses64; exact native sources and covers.')
