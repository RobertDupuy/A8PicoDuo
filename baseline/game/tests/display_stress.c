/* Continuous display ownership test. Tests actual 6502 writes, not only snapshots. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atari.h"
#include "cpu.h"
#include "antic.h"
#include "memory.h"
#include "libatari800/libatari800.h"
#include "atari_labels.h"
void bridge_open(void);int bridge_read(unsigned,unsigned char*);int bridge_write(unsigned,const unsigned char*);int bridge_read_at(unsigned,unsigned,unsigned char*);
static unsigned char mailbox[256];static unsigned ready,commands,front_writes,late_pending,hud_writes_visible;
static int running;static unsigned rng=0x8ad00;
static unsigned clocks(void){return Atari800_nframes*Atari800_tv_mode*114+ANTIC_ypos*114+ANTIC_XPOS;}
void hg_monitor_write(UWORD a,UBYTE v){
 if(!running)return;
 if(a>=0x7a00&&a<0x7a28&&ANTIC_ypos>=8&&ANTIC_ypos<248)hud_writes_visible++;
 unsigned front=(unsigned)MEMORY_mem[L_activefont]<<8;
 if((front==0x4000||front==0x4800)&&a>=front&&a<front+1664){
  if(front_writes<5)fprintf(stderr,"ACTIVE BUFFER WRITE at %04x PC=%04x raster=%d pending=%u active=%02x back=%02x value=%02x\n",a,CPU_regPC,ANTIC_ypos,MEMORY_mem[L_pending],MEMORY_mem[L_activefont],MEMORY_mem[L_backfont],v);
  front_writes++;
 }
}
UBYTE hg_cart_read(UWORD a){return clocks()<ready?0xff:mailbox[a&255];}
void hg_cart_write(UWORD a,UBYTE v){
 a&=255;mailbox[a]=v;if(a!=0xdf)return;unsigned s=mailbox[1]+256*mailbox[2];
 if(v==0x21)mailbox[1]=bridge_read(s,mailbox+2);
 else if(v==0x22){if(running&&MEMORY_mem[L_pending])late_pending++;mailbox[1]=bridge_write(s,mailbox+4);commands++;}
 else if(v==0x23)mailbox[1]=bridge_read_at(0,16,mailbox+2)==16?0:1;
 else abort();
 mailbox[0]=0x11;rng=rng*1664525+1013904223;
 /* Vary long work and following short work to expose VBI reuse races. */
 ready=clocks()+(v==0x22?12000+(rng%78001):(rng%501));
}
int main(int argc,char**argv){
 assert(argc==4);char*opts[]={"atari800","-xl","-xlxe_rom",argv[1],"-xl-rev","custom","-nobasic","-nopatchall",argv[2],"-config","/dev/null"};
 bridge_open();mailbox[0]=0x11;assert(libatari800_init(sizeof(opts)/sizeof(opts[0]),opts));input_template_t in;libatari800_clear_input_array(&in);
 for(unsigned n=0;n<4000;n++){
  if(!libatari800_next_frame(&in)&&libatari800_error_code!=LIBATARI800_DLIST_ERROR)abort();libatari800_error_code=0;
  if(n==100)running=1;
  in.trig0=n>=110&&n<130;in.select=n>180&&n%120<15;
 }
 printf("%s: %u commands, %u producer requests while pending, %u active-buffer writes, %u HUD writes outside vertical blank\n",argv[2],commands,late_pending,front_writes,hud_writes_visible);
 if(!strcmp(argv[3],"expect-fail"))return front_writes?0:2;
 return front_writes||late_pending||hud_writes_visible?3:0;
}
