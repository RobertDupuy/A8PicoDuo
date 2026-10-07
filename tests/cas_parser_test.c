#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "cas_file.h"
void test_storage_init(const char*);
unsigned test_storage_writes(void);
void test_open_cas(unsigned);
int bridge_read(unsigned,uint8_t*);
int bridge_write(unsigned,const uint8_t*);
unsigned char cart_ram[128*1024],cart_d5xx[256];
char errorBuf[40];
extern const unsigned char os_rom[];
static int load_result,load_calls;
int load_file(char *p){(void)p;load_calls++;if(load_result)memset(cart_ram,0xa5,16384);else strcpy(errorBuf,"Test load error");return load_result;}
int main(int argc,char **argv){
    assert(argc==2);test_storage_init(argv[1]);char path[256],err[40];uint8_t out[128];
    for(unsigned bank=0;bank<2;bank++){
        test_open_cas(bank);assert(bridge_read(1,out)==0&&out[6]==0x4c&&out[7]==0xa0&&out[8]==0xc6);
        assert(bridge_read(2,out)==0&&out[0]==0x55&&out[1]==0x55&&out[2]==0xfc);
        assert(bridge_write(2,out)!=0);assert(test_storage_writes()==0);
        cas_reset();
        const char *names[]={"bad-checksum.cas","turbo.cas","fsk.cas","truncated.cas","missing-header.cas","bad-baud.cas","unknown.cas","empty.cas","too-many.cas","empty-partial.cas","bad-address-047f.cas","bad-address-bf80.cas","bad-address-ff80.cas"};
        for(unsigned i=0;i<sizeof names/sizeof names[0];i++){
            snprintf(path,sizeof path,"/LIBRARY%c/CAS/%s",'A'+bank,names[i]);
            memset(err,0,sizeof err);assert(!cas_prepare(path,err));assert(!cas_active());assert(err[0]&&strlen(err)<40);
            printf("Rejected %s: %s\n",names[i],err);
        }
        snprintf(path,sizeof path,"/LIBRARY%c/CAS/count-zero.cas",'A'+bank);assert(cas_prepare(path,err));cas_reset();
    }
    test_open_cas(0);assert(bridge_read(1,out)==0);cas_reset();
    /* Reuse one FIL for CAS, an ordinary file, and the embedded game. */
    assert(cas_prepare("/LIBRARYA/CAS/BOOTTEST.CAS",err));
    FIL f;UINT got;
    assert(f_open(&f,"/LIBRARYA/CAS/BOOTTEST.CAS",FA_READ)==FR_OK&&cas_is_file(&f));
    assert(f_read(&f,out,4,&got)==FR_OK&&got==4&&out[0]==0x96);
    assert(f_open(&f,"/LIBRARYA/CAS/count-zero.cas",FA_READ)==FR_OK&&!cas_is_file(&f));
    assert(f_read(&f,out,4,&got)==FR_OK&&got==4&&!memcmp(out,"FUJI",4));
    assert(f_close(&f)==FR_OK);
    assert(f_open(&f,"/LIBRARYA/CAS/BOOTTEST.CAS",FA_READ)==FR_OK&&cas_is_file(&f));
    assert(f_open(&f,"/HARDGAME.ATR",FA_READ)==FR_OK&&!cas_is_file(&f));
    assert(f_read(&f,out,4,&got)==FR_OK&&got==4&&out[0]==0x96);
    assert(f_close(&f)==FR_OK);cas_reset();
    int type;
    cas_menu_select("/LIBRARYA/CAS/BOOTTEST.CAS","BOOTTEST.cas",&type);
    assert(type==254&&cart_d5xx[1]==3&&cas_active());
    load_calls=0;cas_menu_load_os();assert(load_calls==0&&cart_d5xx[1]==0);
    assert(memcmp(cart_ram+0x6a0,os_rom+0x6a0,627)!=0);
    assert(!memcmp(cart_ram,os_rom,0x6a0)&&!memcmp(cart_ram+0x6a0+627,os_rom+0x6a0+627,16384-0x6a0-627));
    cas_menu_select("/LIBRARYA/CAS/fsk.cas","fsk.CAS",&type);
    assert(type==0&&cart_d5xx[1]==4&&!cas_active()&&strstr((char*)cart_d5xx+2,"FSK"));
    cas_menu_select("/LIBRARYA/test.ATR","test.ATR",&type);
    assert(type==254&&cart_d5xx[1]==3&&!cas_active());
    load_result=0;cas_menu_load_os();assert(!memcmp(cart_ram,os_rom,16384));
    load_result=1;cas_menu_load_os();assert(cart_ram[0]==0xa5&&cart_ram[16383]==0xa5);
    load_result=255;cas_menu_select("test.XEX","test.XEX",&type);assert(type==255&&cart_d5xx[1]==2);
    load_result=8;cas_menu_select("test.CAR","test.CAR",&type);assert(type==8&&cart_d5xx[1]==1);
    load_result=0;cas_menu_select("bad.ROM","bad.ROM",&type);assert(type==0&&cart_d5xx[1]==4);
    assert(test_storage_writes()==0);
    puts("PASS: real two-bank FatFs, CAS validation, bounds, 256-block header, read-only transport, recovery after every invalid file; zero storage writes.");
}
