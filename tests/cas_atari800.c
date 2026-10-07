#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atari.h"
#include "cpu.h"
#include "antic.h"
#include "memory.h"
#include "pia.h"
#include "libatari800/libatari800.h"
void test_storage_init(const char*);
void test_open_cas(unsigned);
unsigned test_storage_writes(void);
int bridge_read(unsigned,unsigned char*);
int bridge_write(unsigned,const unsigned char*);
static unsigned char mailbox[256],os[16384];
static unsigned reads,ram_os;
static unsigned long ready;
void hg_monitor_write(UWORD a,UBYTE v){(void)a;(void)v;}
static unsigned long clocks(void){return (unsigned long)Atari800_nframes*Atari800_tv_mode*114+ANTIC_ypos*114+ANTIC_XPOS;}
UBYTE hg_cart_read(UWORD a){return clocks()<ready?0xff:mailbox[a&255];}
void hg_cart_write(UWORD address,UBYTE value){
    unsigned a=address&255;mailbox[a]=value;if(a!=0xdf)return;
    if(!ram_os){
        /* Match the existing menu's OS-in-RAM state at the start of ATR boot.
         * The cartridge engine/command transport is not cycle-emulated here. */
        PIA_PutByte(0xd303,0x38); /* PORTB direction register */
        PIA_PutByte(0xd301,0xff);
        PIA_PutByte(0xd303,0x3c);
        PIA_PutByte(0xd301,PIA_PORTB&~1);
        MEMORY_SetRAM(0xc000,0xcfff);MEMORY_SetRAM(0xd800,0xffff);
        memcpy(MEMORY_mem+0xc000,os,0x1000);memcpy(MEMORY_mem+0xd800,os+0x1800,0x2800);
        ram_os=1;
    }
    unsigned sector=mailbox[1]+256*mailbox[2];
    if(value==0x21){mailbox[1]=bridge_read(sector,mailbox+2);reads++;if(reads<5)fprintf(stderr,"READ sector %u rc %u data %02x %02x %02x %02x %02x %02x %02x %02x %02x PC %04x\n",sector,mailbox[1],mailbox[2],mailbox[3],mailbox[4],mailbox[5],mailbox[6],mailbox[7],mailbox[8],mailbox[9],mailbox[10],CPU_regPC);}
    else if(value==0x22)mailbox[1]=bridge_write(sector,mailbox+4);
    else if(value==0x23){memset(mailbox+2,0,16);mailbox[2]=0x96;mailbox[3]=2;mailbox[6]=128;mailbox[1]=0;}
    else{fprintf(stderr,"Unexpected cmd %02x PC %04x\n",value,CPU_regPC);exit(3);}
    mailbox[0]=0x11;ready=clocks()+12000; /* Deliberately slow remote-storage transaction. */
}
int main(int argc,char **argv){
    assert(argc==5||argc==6);int external=argc==6;unsigned bank=atoi(argv[4]);test_storage_init(argv[1]);test_open_cas(bank);mailbox[0]=0x11;
    FILE *rom=fopen(argv[2],"rb");assert(rom&&fread(os,1,sizeof os,rom)==sizeof os);fclose(rom);
    char *args[]={"atari800","-xl","-xlxe_rom",argv[2],"-xl-rev","custom","-nobasic","-nopatchall",argv[3],"-config","/dev/null"};
    assert(libatari800_init(sizeof(args)/sizeof(args[0]),args));
    input_template_t input;libatari800_clear_input_array(&input);unsigned char *r=libatari800_get_main_memory_ptr();
    unsigned frames;
    for(frames=0;frames<600;frames++){
        if(external)input.keychar=frames==60?'A':0;
        if(!libatari800_next_frame(&input)&&libatari800_error_code!=LIBATARI800_DLIST_ERROR){fprintf(stderr,"Emulator error %s PC %04x\n",libatari800_error_message(),CPU_regPC);return 4;}
        libatari800_error_code=0;
        if(external?r[0x060d]==0x5c:r[0x0609]==0xa4)break;
    }
    if(external){
        printf("Published BL/C 0.2 loader in FUJI container, %s library %c: frame %u PC %04x reads %u marker %02x\n",argv[3],'A'+bank,frames,CPU_regPC,reads,r[0x60d]);fflush(stdout);
        assert(frames<600&&r[0x60d]==0x5c);assert(test_storage_writes()==0);
        puts("PASS: independent loader boots, accepts A, reads appended binary through C:, executes RUNAD; zero storage writes.");return 0;
    }
    printf("%s library %c: frame %u PC %04x reads %u PORTB %02x BOOT %u; markers",argv[3],'A'+bank,frames,CPU_regPC,reads,PIA_PORTB,r[9]);
    for(unsigned i=0;i<10;i++)printf(" %02x",r[0x600+i]);puts("");fflush(stdout);
    if(frames==600){for(unsigned a=0x400;a<0x410;a++)fprintf(stderr," %02x",r[a]);fprintf(stderr,"\n0700:");for(unsigned a=0x700;a<0x710;a++)fprintf(stderr," %02x",r[a]);fprintf(stderr,"\nC6A0:");for(unsigned a=0xc6a0;a<0xc6b0;a++)fprintf(stderr," %02x",r[a]);fprintf(stderr,"\nvecs:");for(unsigned a=0xe459;a<0xe47f;a++)fprintf(stderr," %02x",r[a]);fprintf(stderr,"\n");}
    assert(frames<600);assert(r[0x600]==0xa1&&r[0x601]==0xa2&&r[0x602]==0xa3);
    assert(r[0x603]==1&&r[0x604]==1&&r[0x605]==136&&r[0x606]==1&&r[0x607]==1&&r[0x608]=='Z');
    assert(!memcmp(r+0x2000,"HELLO",5)&&!memcmp(r+0x2103,"RAW SIO RECORD",14));
    assert(r[0x60a]==146&&r[0x60b]==1&&r[0x60c]==138);
    unsigned screen=r[88]+256*r[89]+r[82];const char *message="A8DUO CAS TEST: PASS";
    for(unsigned i=0;message[i];i++)assert(r[screen+i]==(unsigned char)(message[i]-32));
    assert(r[9]==2&&!(PIA_PORTB&1));assert(test_storage_writes()==0);
    puts("PASS: actual CAS/FatFs/ATR code + Atari800 CPU/ANTIC; low-memory boot, continuation, CASINI, CIO C: partial block+EOF, raw SIO and RBLOKV; zero flash writes.");
}
