/* Test-only D5xx adapter for Atari800 7.2.1, not cartridge firmware. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "atari.h"
#include "cpu.h"
#include "antic.h"
#include "screen.h"
#include "colours.h"
#include "libatari800/libatari800.h"
#include "atari_labels.h"
#include "generated/display.h"
void hg_monitor_write(UWORD a, UBYTE v){(void)a;(void)v;}
void bridge_open(void);
int bridge_read(unsigned,unsigned char*);
int bridge_write(unsigned,const unsigned char*);
int bridge_read_at(unsigned,unsigned,unsigned char*);
unsigned bridge_flash_calls(void);
static unsigned char mailbox[256];
static unsigned reads,writes,frames;
static unsigned long ready;
static unsigned long clocks(void){return (unsigned long)Atari800_nframes*Atari800_tv_mode*114+ANTIC_ypos*114+ANTIC_XPOS;}
UBYTE hg_cart_read(UWORD address){return clocks()<ready?0xff:mailbox[address&255];}
void hg_cart_write(UWORD address,UBYTE value){
 unsigned a=address&255;mailbox[a]=value;if(a!=0xdf)return;
 unsigned sector=mailbox[1]+256*mailbox[2];
 if(value==0x21){mailbox[1]=bridge_read(sector,mailbox+2);reads++;if(sector==1001)frames++;}
 else if(value==0x22){mailbox[1]=bridge_write(sector,mailbox+4);writes++;}
 else if(value==0x23){mailbox[1]=bridge_read_at(0,16,mailbox+2)==16?0:1;}
 else {fprintf(stderr,"Unexpected cartridge command %02x PC %04x\n",value,CPU_regPC);exit(3);}
 mailbox[0]=0x11;
 ready=clocks()+(value==0x22?20000:100); /* modeled command latency: ~11 ms at Atari clock */
}
static void screenshot(const char *path){
 char pal[1024];snprintf(pal,sizeof pal,"%s.palette.ppm",path);FILE*pf=fopen(pal,"wb");fprintf(pf,"P6\n256 256\n255\n");
 for(unsigned y=0;y<256;y++)for(unsigned x=0;x<256;x++){unsigned c=Colours_table[(y/16)*16+x/16];fputc(c>>16,pf);fputc(c>>8,pf);fputc(c,pf);}fclose(pf);

 FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n384 240\n255\n");unsigned char*p=libatari800_get_screen_ptr();
 for(unsigned i=0;i<384*240;i++){unsigned c=Colours_table[p[i]];fputc(c>>16,f);fputc(c>>8,f);fputc(c,f);}fclose(f);
}
static void check_display(void){
 unsigned char*r=libatari800_get_main_memory_ptr();
 assert(r[0x7b00]==0x70&&r[0x7b01]==0x70&&r[0x7b02]==0x70); /* 24 top blank lines */
 assert(r[0x7b03]==0xc2); /* HUD first */
 unsigned char*p=libatari800_get_screen_ptr();unsigned count=0,xmin=384,xmax=0,ymin=240,ymax=0;
 for(unsigned y=0;y<240;y++)for(unsigned x=0;x<384;x++)if(p[y*384+x]==0x48){
  count++;if(x<xmin)xmin=x;if(x>xmax)xmax=x;if(y<ymin)ymin=y;if(y>ymax)ymax=y;
 }
 assert(count==64&&xmax-xmin+1==8&&ymax-ymin+1==8); /* 4x4 logical player */
 assert(ymin>=32&&ymax<208);
 for(unsigned y=0;y<24;y++)for(unsigned x=32;x<352;x++)assert(p[y*384+x]==0x0e);
}
int main(int argc,char **argv){
 assert(argc==4);char*opts[]={"atari800","-xl","-xlxe_rom",argv[1],"-xl-rev","custom","-nobasic","-nopatchall",argv[2],"-config","/dev/null"};
 bridge_open();mailbox[0]=0x11;assert(libatari800_init(sizeof(opts)/sizeof(opts[0]),opts));
 input_template_t input;libatari800_clear_input_array(&input);unsigned char*r=libatari800_get_main_memory_ptr();
 unsigned boot_frame=0;
 for(unsigned i=0;i<600;i++){
  if(!libatari800_next_frame(&input)&&libatari800_error_code!=LIBATARI800_DLIST_ERROR){fprintf(stderr,"Emulator error %s at %04x\n",libatari800_error_message(),CPU_regPC);return 4;}
  libatari800_error_code=0; /* blank startup before OS installs a display list */
  if(r[L_fps]==(strcmp(argv[2],"-pal")?60:50)&&r[0x800]=='H'&&r[0x801]=='G'&&r[0x802]==HG_PROTOCOL_VERSION&&r[0x7a00]==16){boot_frame=i;break;}
 }
 if(!boot_frame){fprintf(stderr,"Boot failed PC %04x fps %u reads %u writes %u state %u clock %u\n",CPU_regPC,r[L_fps],reads,writes,r[L_state],r[L_clock]);screenshot(argv[3]);return 5;}
 for(unsigned i=0;i<12;i++)assert(libatari800_next_frame(&input));
 assert(r[L_state]==0);input.trig0=1;for(int i=0;i<6;i++)assert(libatari800_next_frame(&input));input.trig0=0;
 for(unsigned i=0;i<12;i++)assert(libatari800_next_frame(&input));assert(r[L_state]==1);
 unsigned before=frames;
 for(unsigned i=0;i<120;i++)assert(libatari800_next_frame(&input));
 printf("120 TV frames: %u displayed frames; PC %04x state %u clock %u\n",frames-before,CPU_regPC,r[L_state],r[L_clock]);
 check_display();
 screenshot(argv[3]);
 for(unsigned level=0;level<30;level++){
  if(level){input.select=1;for(unsigned i=0;i<6;i++)assert(libatari800_next_frame(&input));input.select=0;for(unsigned i=0;i<6;i++)assert(libatari800_next_frame(&input));}
  assert(r[L_level]==level);assert(r[L_state]==0||r[L_state]==1);
  if(r[L_state]==0){input.trig0=1;for(unsigned i=0;i<6;i++)assert(libatari800_next_frame(&input));input.trig0=0;for(unsigned i=0;i<6;i++)assert(libatari800_next_frame(&input));}
  assert(r[L_state]==1);unsigned n=frames;
  for(unsigned i=0;i<120;i++)assert(libatari800_next_frame(&input));
  assert(frames-n>=39);assert(r[L_state]!=4);
 }
 assert(bridge_flash_calls()==0);
 printf("PASS: Atari800 %s, original patched OS boot in %u TV frames; %u reads, %u commands; all 30 levels animate with ANTIC/GTIA DMA, 20000-cycle write latency, 100-cycle cached read latency; zero flash calls\n",argv[2],boot_frame,reads,writes);
 return 0;
}
