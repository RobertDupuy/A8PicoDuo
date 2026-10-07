/* A8Duo storage glue. Atari engine and its SRAM buffers are unchanged. */
#include "fatfs_disk.h"
#include "ff.h"
#include "diskio.h"
#include "tusb.h"
#include <string.h>
#ifndef DUO_COMPANION
#include "block_client.h"
#endif
static bool mounted,remote;
bool mount_fatfs_disk(void){
 if(!mounted)mounted=flash_fs_mount()==0;
#ifndef DUO_COMPANION
 if(!tud_mounted()&&!remote)remote=nx_storage_open();
#endif
 return mounted;
}
bool fatfs_is_mounted(void){return mounted;}
bool duo_remote_ready(void){return remote;}
void create_fatfs_disk(void){
 /* Upstream invokes this after mount failure, including at USB attach. Only a
    completely blank data area may be automatically formatted. */
 if(mounted||!flash_fs_blank())return;
 flash_fs_create();mounted=true;FATFS fs;BYTE work[512];
 if(f_mkfs("0:",0,work,sizeof work)!=FR_OK){mounted=false;return;}
 if(f_mount(&fs,"0:",1)!=FR_OK){mounted=false;return;}
 f_setlabel("0:A8DUO");f_mount(0,"0:",0);flash_fs_sync();
}
static bool valid(uint32_t sector,uint32_t count){return count&&sector<SECTOR_NUM&&count<=SECTOR_NUM-sector;}
uint32_t fatfs_disk_read(uint8_t *b,uint32_t sector,uint32_t count){
 if(!mounted)return RES_NOTRDY;if(!valid(sector,count))return RES_PARERR;
 for(uint32_t i=0;i<count;i++)flash_fs_read_FAT_sector(sector+i,b+512*i);return RES_OK;
}
uint32_t fatfs_disk_write(const uint8_t *b,uint32_t sector,uint32_t count){
 if(!mounted)return RES_NOTRDY;if(!valid(sector,count))return RES_PARERR;
 for(uint32_t i=0;i<count;i++){flash_fs_write_FAT_sector(sector+i,b+512*i);if(!flash_fs_verify_FAT_sector(sector+i,b+512*i))return RES_ERROR;}return RES_OK;
}
void fatfs_disk_sync(void){if(mounted)flash_fs_sync();}
bool fatfs_disk_sync_ok(void){if(!mounted)return false;flash_fs_sync();return true;}
