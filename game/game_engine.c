/* Original 30-level timeline adapter and ANTIC glyph composer for U2.
 * Does not touch flash, GPIO, interrupts, the Atari bus, or the storage driver.
 * Atari owns joystick polling, video DMA, player sprite and POKEY output.
 */
#include "game_engine.h"
#include <string.h>
#include <stdlib.h>
typedef struct {int32_t a,b,c,d,x,y;} Matrix;
typedef struct {uint16_t track,frames,child,count;} Node;
typedef struct {int32_t x,y;} Point;
typedef struct {int32_t x1,y1,x2,y2,x,y;} Check;
typedef struct {uint32_t root,bg,mask,coin,ncoin,required,check,ncheck;} Level;
#include "generated/levels.h"
#include "generated/music.h"
#include "generated/display.h"
#define W HG_SCREEN_WIDTH
#define H HG_SCREEN_HEIGHT
#define Q 65536
static uint8_t background[W*H],pixels[W*H],wallmask[27500],snapshot[HG_FRAME_SIZE];
static uint8_t collected[67],level,state,fps,checkpoint,dying,events,prev_buttons,mute;
static uint16_t deaths,fraction,glyph_count,hazard_count;
static uint32_t tick,music_tick;
static int32_t px,py;
static Point enemy[320];
static uint8_t active;
static int32_t qm(int32_t a,int32_t b){return (int32_t)(((int64_t)a*b)>>16);}
static Matrix compose(Matrix a,Matrix b){return (Matrix){qm(a.a,b.a)+qm(a.c,b.b),qm(a.b,b.a)+qm(a.d,b.b),qm(a.a,b.c)+qm(a.c,b.d),qm(a.b,b.c)+qm(a.d,b.d),qm(a.a,b.x)+qm(a.c,b.y)+a.x,qm(a.b,b.x)+qm(a.d,b.y)+a.y};}
static void visit(unsigned i,Matrix parent){
 const Node*n=nodes+i;Matrix m=compose(parent,tracks[n->track+tick%n->frames]);
 if(!n->count){if(hazard_count<320)enemy[hazard_count++]=(Point){m.x,m.y};return;}
 for(unsigned k=0;k<n->count;k++)visit(children[n->child+k],m);
}
static void enemies(void){hazard_count=0;visit(levels[level].root,(Matrix){Q,0,0,Q,0,0});}
static void unpack(uint8_t*out,unsigned size,const uint8_t*in){unsigned p=0;while(p<size){unsigned n=*in++,v=*in++;if(n>size-p)n=size-p;memset(out+p,v,n);p+=n;}}
static int wall(int32_t x,int32_t y){int xx=x/Q,yy=y/Q;if(xx<0||xx>=550||yy<0||yy>=400)return 1;unsigned n=yy*550+xx;return (wallmask[n>>3]>>(7-(n&7)))&1;}
static int sx(int32_t x){return HG_WORLD_LEFT+(int)(((int64_t)x*HG_WORLD_WIDTH/HG_SOURCE_WIDTH+Q/2)/Q);}
static int sy(int32_t y){return (int)(((int64_t)(y-HG_SOURCE_TOP*Q)*H/HG_SOURCE_HEIGHT+Q/2)/Q);}
static void dot(int x,int y,uint8_t color){
 for(int yy=-1;yy<=2;yy++)for(int xx=-1;xx<=2;xx++)
  if(!((xx==-1||xx==2)&&(yy==-1||yy==2))&&x+xx>=0&&x+xx<W&&y+yy>=0&&y+yy<H)pixels[(y+yy)*W+x+xx]=color;
}
static unsigned hash8(const uint8_t*p){unsigned h=2166136261u;for(int k=0;k<8;k++)h=(h^p[k])*16777619u;return h;}
static void render(void){
 memcpy(pixels,background,sizeof pixels);
 const Level*l=levels+level;
 for(unsigned i=0;i<l->ncoin;i++)if(!collected[i])dot(sx(coins[l->coin+i].x),sy(coins[l->coin+i].y),4);
 for(unsigned i=0;i<hazard_count;i++)dot(sx(enemy[i].x),sy(enemy[i].y),2);
 memset(snapshot,0,sizeof snapshot);uint8_t*font=snapshot+128,*screen=snapshot+1152;
 int16_t ht[512];for(unsigned k=0;k<512;k++)ht[k]=-1;glyph_count=0;
 for(int cy=0;cy<HG_CHAR_ROWS;cy++)for(int cx=0;cx<40;cx++){
  uint8_t glyph[8]={0};int yellow=0;
  for(int yy=0;yy<8;yy++)for(int xx=0;xx<4;xx++)if(pixels[(cy*8+yy)*W+cx*4+xx]==4)yellow=1;
  for(int yy=0;yy<8;yy++)for(int xx=0;xx<4;xx++){
   unsigned c=pixels[(cy*8+yy)*W+cx*4+xx];
   /* Original coins are on floor. If a rounded glyph crosses a safe edge,
      retain a pale floor behind that yellow coin (collision remains exact). */
   if(c==4)c=3;else if(c==3&&yellow)c=0;
   glyph[yy]|=c<<(6-xx*2);
  }
  unsigned h=hash8(glyph)&511;
  while(ht[h]>=0&&memcmp(font+ht[h]*8,glyph,8))h=(h+1)&511;
  if(ht[h]<0){if(glyph_count==128){state=4;break;}ht[h]=glyph_count;memcpy(font+glyph_count*8,glyph,8);glyph_count++;}
  screen[cy*40+cx]=ht[h]|(yellow?128:0);
 }
 snapshot[0]='H';snapshot[1]='G';snapshot[2]=HG_PROTOCOL_VERSION;snapshot[3]=state==4?1:0;snapshot[4]=level;snapshot[5]=state;
 snapshot[6]=sx(px)-HG_PLAYER_WIDTH/2;snapshot[7]=sy(py)-HG_PLAYER_HEIGHT/2;snapshot[8]=deaths%10;snapshot[9]=deaths/10%10;snapshot[10]=deaths/100%10;snapshot[11]=deaths/1000%10;
 unsigned n=0;for(unsigned i=0;i<l->ncoin;i++)n+=collected[i];snapshot[12]=n;snapshot[13]=l->required;
 snapshot[14]=events;snapshot[15]=dying;snapshot[16]=mute;snapshot[17]=glyph_count;snapshot[18]=tick;snapshot[19]=tick>>8;
 snapshot[20]=hazard_count;snapshot[21]=hazard_count>>8;
 snapshot[22]=level<19?0x84:(level<29?0x64:0x48);
 memcpy(snapshot+24,music+(music_tick%(sizeof(music)/6))*6,6);
}
static void spawn(void){const Check*c=checks+levels[level].check+checkpoint;px=c->x;py=c->y;}
static void load(unsigned n){level=n;state=0;checkpoint=0;dying=0;tick=0;fraction=0;memset(collected,0,sizeof collected);unpack(background,sizeof background,background_rle+levels[n].bg);unpack(wallmask,sizeof wallmask,collision_rle+levels[n].mask);spawn();enemies();}
static int overlaps(const Check*c){return px+HG_PLAYER_HALF_Q>c->x1&&px-HG_PLAYER_HALF_Q<c->x2&&py+HG_PLAYER_HALF_Q>c->y1&&py-HG_PLAYER_HALF_Q<c->y2;}
static int collision(void){
 static const int16_t probes[5][2]={{0,0},{HG_PLAYER_HALF_16,0},{-HG_PLAYER_HALF_16,0},{0,HG_PLAYER_HALF_16},{0,-HG_PLAYER_HALF_16}};
 for(unsigned i=0;i<hazard_count;i++){
  int dx=(enemy[i].x-px)>>12,dy=(enemy[i].y-py)>>12; // 1/16 original pixel
  if(abs(dx)>HG_PLAYER_HALF_16+104||abs(dy)>HG_PLAYER_HALF_16+104)continue;
  for(int p=0;p<5;p++){int x=dx-probes[p][0],y=dy-probes[p][1];if(x*x+y*y<=104*104)return 1;}
 }return 0;
}
static void game_tick(uint8_t stick){
 tick++;enemies();
 if(dying){if(--dying==0){deaths=(deaths+1)%10000;memset(collected,0,sizeof collected);spawn();}return;}
 const Level*l=levels+level;
 /* Original checkpoints advance in order; coins reset on every death. */
 for(unsigned i=checkpoint+1;i+1<l->ncheck;i++)if(overlaps(checks+l->check+i)){checkpoint=i;events|=8;}
 unsigned have=0;for(unsigned i=0;i<l->ncoin;i++)have+=collected[i];
 if(have==l->required&&overlaps(checks+l->check+l->ncheck-1)){state=level==29?3:2;events|=4;return;}
 int32_t ox=px,oy=py;
 if(stick&8)px+=3*Q;if(stick&4)px-=3*Q;
 if(wall(px+HG_PLAYER_HALF_Q-3*Q,py)||wall(px-HG_PLAYER_HALF_Q+3*Q,py))px=ox;
 if(stick&1)py-=3*Q;if(stick&2)py+=3*Q;
 if(wall(px,py-HG_PLAYER_HALF_Q+3*Q)||wall(px,py+HG_PLAYER_HALF_Q-3*Q))py=oy;
 if(collision()){dying=26;events|=1;return;}
 for(unsigned i=0;i<l->ncoin;i++)if(!collected[i]&&abs(px-coins[l->coin+i].x)<(HG_PLAYER_HALF_Q+13*Q/2)&&abs(py-coins[l->coin+i].y)<(HG_PLAYER_HALF_Q+13*Q/2)){collected[i]=1;events|=2;}
}
bool hg_command(const uint8_t in[8],uint8_t out[512]){
 if(in[0]!=HG_PROTOCOL_VERSION||(in[1]!=50&&in[1]!=60)||in[2]>12||in[3]>15||in[4]>15||in[5]>=30||in[6]||in[7])return false;
 events=0;fps=in[1];uint8_t edge=in[4]&~prev_buttons;prev_buttons=in[4];
 if(!active){active=1;deaths=0;mute=0;load(in[5]);}
 if(edge&2)load(level); // START retries the current level
 if(edge&4){load((level+1)%30);} // SELECT: practice level selection
 if(edge&8)mute^=1;
 if(edge&1){if(state==0||state==5)state=1;else if(state==1)state=5;else if(state==2)load(level+1);else if(state==3){deaths=0;load(0);}else load(level);}
 fraction+=in[2]*30;while(fraction>=fps){fraction-=fps;music_tick++;if(state==1)game_tick(in[3]);}
 render();memcpy(out,snapshot,512);return true;
}
bool hg_page(uint8_t page,uint8_t out[512]){if(!active||page>3)return false;memcpy(out,snapshot+page*512,512);return true;}
#ifdef HG_TEST
void hg_test_position(int x,int y){px=x*Q;py=y*Q;}
unsigned hg_test_glyphs(void){return glyph_count;}
unsigned hg_test_hazards(void){return hazard_count;}
int hg_test_wall(int x,int y){return wall(x*Q,y*Q);}
void hg_test_tick(unsigned t){tick=t;enemies();render();}
const uint8_t*hg_test_pixels(void){return pixels;}
#endif
#ifdef HG_TEST
void hg_test_level(unsigned l){active=1;load(l);render();}
#endif

#ifdef HG_TEST
int hg_test_enemy_x(unsigned i){return enemy[i].x;}
int hg_test_enemy_y(unsigned i){return enemy[i].y;}
unsigned hg_test_state(void){return state;}
unsigned hg_test_deaths(void){return deaths;}
unsigned hg_test_collected(void){unsigned n=0;for(unsigned i=0;i<levels[level].ncoin;i++)n+=collected[i];return n;}
unsigned hg_test_checkpoint(void){return checkpoint;}
void hg_test_clearcoins(void){memset(collected,1,sizeof collected);}
void hg_test_reset(void){active=prev_buttons=0;}
int hg_test_collision(void){return collision();}
#endif
