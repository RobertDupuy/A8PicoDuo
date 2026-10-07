/* Host-only disk backend. Real FatFs, both real library wrappers, the actual
 * inherited ATR functions, and production CAS code remain in the test path. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ff.h"
#include "diskio.h"
#include "fatfs_disk.h"
#include "cas_file.h"
#include "osrom.h"
static unsigned char *drives[2];
static bool usb=true;
static unsigned writes;
bool tud_mounted(void){return usb;}
bool fatfs_is_mounted(void){return true;}
bool duo_remote_ready(void){return true;}
bool nx_game_command(const uint8_t*a,uint8_t*b){(void)a;(void)b;return false;}
DSTATUS disk_status(BYTE p){return p<2?0:STA_NOINIT;}
DSTATUS disk_initialize(BYTE p){return disk_status(p);}
DRESULT disk_read(BYTE p,BYTE*b,LBA_t l,UINT n){assert(p<2&&n&&l<SECTOR_NUM&&n<=SECTOR_NUM-l);memcpy(b,drives[p]+l*512,n*512);return RES_OK;}
DRESULT disk_write(BYTE p,const BYTE*b,LBA_t l,UINT n){assert(p<2&&n&&l<SECTOR_NUM&&n<=SECTOR_NUM-l);memcpy(drives[p]+l*512,b,n*512);writes++;return RES_OK;}
DRESULT disk_ioctl(BYTE p,BYTE cmd,void*b){if(p>=2)return RES_PARERR;switch(cmd){case CTRL_SYNC:return RES_OK;case GET_SECTOR_COUNT:*(LBA_t*)b=SECTOR_NUM;return RES_OK;case GET_SECTOR_SIZE:*(WORD*)b=512;return RES_OK;case GET_BLOCK_SIZE:*(DWORD*)b=8;return RES_OK;default:return RES_PARERR;}}
void test_storage_init(const char *directory){
    FATFS fs[2];uint8_t work[4096],buffer[512];char path[512];
    for(unsigned bank=0;bank<2;bank++){
        drives[bank]=calloc(SECTOR_NUM,512);assert(drives[bank]);
        snprintf(path,sizeof path,"%u:",bank);assert(f_mkfs(path,0,work,sizeof work)==FR_OK);assert(f_mount(&fs[bank],path,1)==FR_OK);
        snprintf(path,sizeof path,"%u:/CAS",bank);assert(f_mkdir(path)==FR_OK);
        /* POSIX DIR conflicts with FatFs DIR; enumerate fixture names below. */
        const char *names[]={"BOOTTEST.CAS","bad-checksum.cas","turbo.cas","fsk.cas","truncated.cas","missing-header.cas","bad-baud.cas","unknown.cas","empty.cas","too-many.cas","empty-partial.cas","bad-address-047f.cas","bad-address-bf80.cas","bad-address-ff80.cas","count-zero.cas"};
        for(unsigned i=0;i<sizeof names/sizeof names[0];i++){
            snprintf(path,sizeof path,"%s/%s",directory,names[i]);FILE *in=fopen(path,"rb");assert(in);
            snprintf(path,sizeof path,"%u:/CAS/%s",bank,names[i]);FIL f;assert(f_open(&f,path,FA_CREATE_ALWAYS|FA_WRITE)==FR_OK);
            size_t n;while((n=fread(buffer,1,sizeof buffer,in))){UINT done;assert(f_write(&f,buffer,n,&done)==FR_OK&&done==n);}
            assert(fclose(in)==0);assert(f_close(&f)==FR_OK);
        }
        snprintf(path,sizeof path,"%u:",bank);assert(f_mount(NULL,path,0)==FR_OK);
    }
    usb=false;writes=0;
}
unsigned test_storage_writes(void){return writes;}
int mount_atr(char*);
int read_atr_sector(uint16_t,uint8_t,uint8_t*);
int write_atr_sector(uint16_t,uint8_t,uint8_t*);
void test_open_cas(unsigned bank){char path[64],error[40];snprintf(path,sizeof path,"/LIBRARY%c/CAS/BOOTTEST.CAS",'A'+bank);assert(cas_prepare(path,error));assert(mount_atr(path)==0);}
int bridge_read(unsigned sector,uint8_t *buf){return read_atr_sector(sector,0,buf);}
int bridge_write(unsigned sector,const uint8_t *buf){return write_atr_sector(sector,0,(uint8_t*)buf);}
