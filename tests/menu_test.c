#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "diskio.h"
#include "fatfs_disk.h"
#include "osrom.h"
#include "../game/game_engine.h"
bool nx_game_command(const uint8_t*in,uint8_t*b){if(!hg_command(in,b))return false;for(int p=1;p<4;p++)if(!hg_page(p,b+p*512))return false;return true;}
static unsigned char *drives[2];static bool usb=true;static bool local=true,remote=true;
bool tud_mounted(void){return usb;}
bool fatfs_is_mounted(void){return local;}
bool duo_remote_ready(void){return remote;}
DSTATUS disk_status(BYTE p){return p<2?0:STA_NOINIT;}
DSTATUS disk_initialize(BYTE p){return disk_status(p);}
DRESULT disk_read(BYTE p,BYTE *b,LBA_t l,UINT n){assert(p<2&&n&&l<SECTOR_NUM&&n<=SECTOR_NUM-l);memcpy(b,drives[p]+l*512,n*512);return RES_OK;}
DRESULT disk_write(BYTE p,const BYTE *b,LBA_t l,UINT n){assert(p<2&&n&&l<SECTOR_NUM&&n<=SECTOR_NUM-l);memcpy(drives[p]+l*512,b,n*512);return RES_OK;}
DRESULT disk_ioctl(BYTE p,BYTE cmd,void *b){if(p>=2)return RES_PARERR;switch(cmd){case CTRL_SYNC:return RES_OK;case GET_SECTOR_COUNT:*(LBA_t*)b=SECTOR_NUM;return RES_OK;case GET_SECTOR_SIZE:*(WORD*)b=512;return RES_OK;case GET_BLOCK_SIZE:*(DWORD*)b=8;return RES_OK;default:return RES_PARERR;}}
int main(void){FATFS fs[2];FIL file;BYTE work[4096],out[512];UINT n;char path[64];
 for(int i=0;i<2;i++){drives[i]=calloc(SECTOR_NUM,512);assert(drives[i]);sprintf(path,"%d:",i);assert(f_mkfs(path,0,work,sizeof work)==FR_OK);assert(f_mount(&fs[i],path,1)==FR_OK);sprintf(path,"%d:/GAMES",i);assert(f_mkdir(path)==FR_OK);sprintf(path,"%d:/GAMES/Test Game.car",i);assert(f_open(&file,path,FA_CREATE_ALWAYS|FA_WRITE)==FR_OK);memset(out,0x40+i,512);assert(f_write(&file,out,512,&n)==FR_OK&&n==512);assert(f_close(&file)==FR_OK);sprintf(path,"%d:",i);assert(f_mount(0,path,0)==FR_OK);}
 usb=false;FATFS temporary;assert(f_mount(&temporary,"",1)==FR_OK);DIR root,sub;FILINFO info;
 assert(f_opendir(&root,"")==FR_OK);assert(f_readdir(&root,&info)==FR_OK&&!strcmp(info.altname,"LIBRARYA"));
 assert(f_opendir(&sub,"/LIBRARYA/GAMES")==FR_OK);assert(f_readdir(&sub,&info)==FR_OK&&!strcmp(info.fname,"Test Game.car"));assert(f_closedir(&sub)==FR_OK);
 assert(f_readdir(&root,&info)==FR_OK&&!strcmp(info.altname,"LIBRARYB"));assert(f_readdir(&root,&info)==FR_OK&&!strcmp(info.altname,"HARDGAME.ATR")&&!(info.fattrib&AM_DIR));assert(f_readdir(&root,&info)==FR_OK&&!info.fname[0]);assert(f_closedir(&root)==FR_OK);
 for(int i=0;i<2;i++){sprintf(path,"/LIBRARY%c/GAMES/Test Game.car",'A'+i);assert(f_open(&file,path,FA_READ|FA_WRITE)==FR_OK);assert(f_read(&file,out,512,&n)==FR_OK&&n==512);for(int j=0;j<512;j++)assert(out[j]==0x40+i);assert(f_lseek(&file,3)==FR_OK);out[0]=0x91;assert(f_write(&file,out,1,&n)==FR_OK&&n==1);assert(f_sync(&file)==FR_OK);assert(f_close(&file)==FR_OK);}
 /* f_mount(NULL) in upstream loader must not invalidate a mounted ATR. */
 assert(f_open(&file,"/LIBRARYB/GAMES/Test Game.car",FA_READ)==FR_OK);assert(f_mount(0,"",1)==FR_OK);assert(f_read(&file,out,4,&n)==FR_OK&&n==4&&out[3]==0x91);assert(f_close(&file)==FR_OK);
 remote=false;assert(f_opendir(&root,"/")==FR_OK);assert(f_readdir(&root,&info)==FR_OK&&!strcmp(info.altname,"LIBRARYA"));assert(f_readdir(&root,&info)==FR_OK&&!strcmp(info.altname,"HARDGAME.ATR")&&!(info.fattrib&AM_DIR));assert(f_readdir(&root,&info)==FR_OK&&!info.fname[0]);assert(f_closedir(&root)==FR_OK);
 local=false;remote=true;assert(f_opendir(&root,"")==FR_OK);assert(f_readdir(&root,&info)==FR_OK&&!strcmp(info.altname,"LIBRARYB"));assert(f_closedir(&root)==FR_OK);
 assert(f_open(&file,"/HARDGAME.ATR",FA_READ|FA_WRITE)==FR_OK);assert(f_read(&file,out,16,&n)==FR_OK&&n==16&&out[0]==0x96&&out[1]==2);
 /* Reusing the FIL object for an ordinary file must clear virtual identity. */
 assert(f_open(&file,"/LIBRARYA/GAMES/Test Game.car",FA_READ)==FR_OK);assert(f_read(&file,out,4,&n)==FR_OK&&n==4&&out[0]==0x40);assert(f_close(&file)==FR_OK);
 char huge[300];memset(huge,'a',299);huge[299]=0;assert(f_open(&file,huge,FA_READ)==FR_INVALID_NAME);
 puts("PASS: real FatFs two-volume format, virtual libraries, nested directories, long names, reads/writes/sync, persistent ATR handles, missing banks, path bounds");
 for(int i=0;i<2;i++)free(drives[i]);return 0;}
