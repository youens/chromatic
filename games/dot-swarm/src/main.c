#include "runtime.h"
#include <string.h>
#define N 4
#define MAXLEN 48
#define FOOD 80
#define W 40
#define H 32
const char game_title[]="DOT SWARM", game_tagline[]="GROW YOUR CODEX DOT",
 game_controls1[]="D PAD STEER",game_controls2[]="HOLD A TO BOOST",
 game_goal[]="GROW TO 48 DOTS";
#define C RGB
const uint16_t game_palette[32]={
 C(1,1,5),C(3,4,10),C(9,13,21),C(26,29,31),
 C(1,1,5),C(0,9,13),C(0,26,28),C(27,31,31),
 C(1,1,5),C(13,1,12),C(31,3,22),C(31,25,31),
 C(1,1,5),C(10,3,18),C(21,8,31),C(31,26,31),
 C(1,1,5),C(15,4,1),C(31,13,2),C(31,28,17),
 C(1,1,5),C(12,7,0),C(31,24,0),C(31,31,23),
 C(1,1,5),C(2,3,8),C(4,7,13),C(10,15,23),
 C(1,1,5),C(0,10,7),C(9,31,15),C(27,31,26)};
typedef struct { uint8_t x[MAXLEN],y[MAXLEN],len,dir,alive; } Snake;
Snake snakes[N];
uint8_t fx[FOOD],fy[FOOD],food_slot,boosting,step_clock,boost_clock,kills;
int8_t camx,camy;
const int8_t dx[8]={1,1,0,-1,-1,-1,0,1};
const int8_t dy[8]={0,1,1,1,0,-1,-1,-1};
uint8_t occupied(uint8_t x,uint8_t y,uint8_t except) {
 uint8_t s,j;Snake *a=snakes;
 for(s=0;s<N;s++,a++)if(s!=except && a->alive)
  for(j=0;j<a->len;j++)if(a->x[j]==x && a->y[j]==y)return 1;
 return 0;
}
void spawn(uint8_t s) {
 uint8_t j,x,y,tries=0;
 do{x=6+random16()%28;y=4+random16()%24;}while(occupied(x,y,s)&&++tries<64);
 snakes[s].len=6;snakes[s].dir=0;snakes[s].alive=1;
 for(j=0;j<MAXLEN;j++){snakes[s].x[j]=x-j%6;snakes[s].y[j]=y;}
}
void game_reset(void) {
 uint8_t i;
 memset(snakes,0,sizeof(snakes));food_slot=boosting=step_clock=boost_clock=kills=0;
 for(i=0;i<N;i++)spawn(i);
 snakes[0].x[0]=20;snakes[0].y[0]=16;
 for(i=1;i<MAXLEN;i++){snakes[0].x[i]=20-i%6;snakes[0].y[i]=16;}
 for(i=0;i<FOOD;i++){fx[i]=1+random16()%(W-2);fy[i]=1+random16()%(H-2);}
 fx[0]=22;fy[0]=16;
}
uint8_t danger(uint8_t s,uint8_t d) {
 int8_t x=snakes[s].x[0]+dx[d],y=snakes[s].y[0]+dy[d];
 return x<1||x>=W-1||y<1||y>=H-1||occupied(x,y,s);
}
void die(uint8_t s) {
 uint8_t j;
 snakes[s].alive=0;
 for(j=0;j<snakes[s].len;j++){
  fx[food_slot]=snakes[s].x[j];fy[food_slot]=snakes[s].y[j];food_slot=(food_slot+1)%FOOD;
 }
 if(!s){ended=1;noise(0x67);}else{kills++;points(50);tone(1750,0x63);}
}
void advance(uint8_t s) {
 uint8_t j,d=snakes[s].dir,tx=1,ty=1,bestd=d;uint16_t nearest=65535,v;
 Snake *a=&snakes[s];
 if(s){
  /* Reconsider a target periodically, then try the preferred heading first. */
  if(!(step_clock&3)){
   for(j=s;j<FOOD;j+=4){
    v=distance(a->x[0],fx[j])+distance(a->y[0],fy[j]);
    if(v<nearest){nearest=v;tx=fx[j];ty=fy[j];}
   }
   if(tx>a->x[0])bestd=ty>a->y[0]?1:ty<a->y[0]?7:0;
   else if(tx<a->x[0])bestd=ty>a->y[0]?3:ty<a->y[0]?5:4;
   else bestd=ty>a->y[0]?2:6;
   if(bestd!=((d+4)&7))d=bestd;
  }
  if(danger(s,d))for(j=1;j<8;j++){
   bestd=(d+j)&7;
   if(bestd!=((a->dir+4)&7)&&!danger(s,bestd)){d=bestd;break;}
  }
  a->dir=d;
 }
 if(danger(s,d)){die(s);return;}
 for(j=a->len-1;j>0;j--){a->x[j]=a->x[j-1];a->y[j]=a->y[j-1];}
 a->x[0]+=dx[d];a->y[0]+=dy[d];
 for(j=0;j<FOOD;j++)if(a->x[0]==fx[j]&&a->y[0]==fy[j]){
  fx[j]=1+random16()%(W-2);fy[j]=1+random16()%(H-2);
  if(a->len<MAXLEN){a->x[a->len]=a->x[a->len-1];a->y[a->len]=a->y[a->len-1];a->len++;}
  if(!s){points(10);tone(1850+a->len,0x62);if(a->len==MAXLEN)ended=won=1;}
 }
}
void game_update(void) {
 uint8_t d=snakes[0].dir,s;
 if(keys&J_UP)d=(keys&J_LEFT)?5:(keys&J_RIGHT)?7:6;
 else if(keys&J_DOWN)d=(keys&J_LEFT)?3:(keys&J_RIGHT)?1:2;
 else if(keys&J_LEFT)d=4;else if(keys&J_RIGHT)d=0;
 if(d!=((snakes[0].dir+4)&7))snakes[0].dir=d;
 boosting=(keys&J_A)&&snakes[0].len>6;
 step_clock++;
 if(!(step_clock&1)){
  advance(0);
  if(boosting && !ended)advance(0);
  if(boosting&&++boost_clock>=3){
   boost_clock=0;
   if(snakes[0].len>6){s=--snakes[0].len;fx[food_slot]=snakes[0].x[s];fy[food_slot]=snakes[0].y[s];food_slot=(food_slot+1)%FOOD;}
  }
 }
 s=1+step_clock%3;{
  if(snakes[s].alive)advance(s);else if(!(random16()&7))spawn(s);
 }
}
void plot(uint8_t x,uint8_t y,uint8_t t,uint8_t p) {
 int8_t sx=(int8_t)x-camx,sy=(int8_t)y-camy;
 if(sx>=0&&sx<20&&sy>=0&&sy<14)tile(sx,sy+2,t,p);
}
void game_draw(void) {
 uint8_t x,y,s,j;Snake *a;
 camx=(int8_t)snakes[0].x[0]-10;camy=(int8_t)snakes[0].y[0]-7;
 if(camx<0)camx=0;if(camx>W-20)camx=W-20;
 if(camy<0)camy=0;if(camy>H-14)camy=H-14;
 screen(0,0);
 for(y=0;y<14;y++)for(x=0;x<20;x++){
  if(x+camx==0||x+camx==W-1||y+camy==0||y+camy==H-1)tile(x,y+2,64+((ticks>>3)&1),2);
  else tile(x,y+2,70+(((uint8_t)(x+camx)&3)==0)+2*(((uint8_t)(y+camy)&3)==0),6);
 }
 for(j=0;j<FOOD;j++)plot(fx[j],fy[j],60+(((ticks>>3)+j)&1)*6,5);
 for(s=N;s>0;s--){
  a=&snakes[s-1];
  if(a->alive)for(j=a->len;j>0;j--)plot(a->x[j-1],a->y[j-1],j==1?80+a->dir:(j==a->len?68:((((uint8_t)(ticks>>2)+j)&7)==0?69:61)),s==1&&boosting?7:s);
 }
 label(0,0,"DOT SWARM",2);label(12,0,"DOTS",4);number(17,0,snakes[0].len,2,1);
 label(0,1,"SCORE",1);number(6,1,score,5,1);label(13,1,"GOAL48",5);
 label(0,16,boosting?"BOOSTING":"A BOOST",2);label(11,16,"KO",4);number(14,16,kills,3,3);
 label(0,17,"OPENAI",1);label(8,17,"DEVDAY",2);label(16,17,"2026",4);
}
