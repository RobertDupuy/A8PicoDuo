/* Two independent FAT volumes: local boot flash and polled companion flash. */
#include "ff.h"
#include "diskio.h"
#include "fatfs_disk.h"
#ifndef DUO_COMPANION
#include "block_client.h"
#endif
DSTATUS disk_status(BYTE p){if(p==0)return fatfs_is_mounted()?0:STA_NOINIT;
#ifndef DUO_COMPANION
 if(p==1)return duo_remote_ready()?0:STA_NOINIT;
#endif
 return STA_NOINIT;}
DSTATUS disk_initialize(BYTE p){return disk_status(p);}
static bool valid(LBA_t s,UINT n){return n&&s<SECTOR_NUM&&n<=SECTOR_NUM-s;}
DRESULT disk_read(BYTE p,BYTE *b,LBA_t s,UINT n){
 if(!valid(s,n))return RES_PARERR;if(p==0)return fatfs_disk_read(b,s,n);
#ifndef DUO_COMPANION
 if(p==1){if(disk_status(p))return RES_NOTRDY;for(UINT i=0;i<n;i++)if(!nx_storage_read(s+i,b+512*i))return RES_ERROR;return RES_OK;}
#endif
 return RES_PARERR;}
DRESULT disk_write(BYTE p,const BYTE *b,LBA_t s,UINT n){
 if(!valid(s,n))return RES_PARERR;if(p==0)return fatfs_disk_write(b,s,n);
#ifndef DUO_COMPANION
 if(p==1){if(disk_status(p))return RES_NOTRDY;for(UINT i=0;i<n;i++)if(!nx_storage_write(s+i,b+512*i))return RES_ERROR;return RES_OK;}
#endif
 return RES_PARERR;}
DRESULT disk_ioctl(BYTE p,BYTE cmd,void *b){
 if(disk_status(p))return RES_NOTRDY;
 if(cmd==CTRL_SYNC){
#ifndef DUO_COMPANION
  if(p==1)return nx_storage_sync()?RES_OK:RES_ERROR;
#endif
  return fatfs_disk_sync_ok()?RES_OK:RES_ERROR;}
 switch(cmd){case GET_SECTOR_COUNT:*(LBA_t*)b=SECTOR_NUM;break;case GET_SECTOR_SIZE:*(WORD*)b=512;break;case GET_BLOCK_SIZE:*(DWORD*)b=8;break;default:return RES_PARERR;}return RES_OK;
}
