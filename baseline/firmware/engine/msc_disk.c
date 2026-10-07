/* A8Next USB block callbacks. Synchronous writes, explicit errors and safe eject. */
#include "tusb.h"
#include "fatfs_disk.h"
#include <string.h>
static bool ejected;
void tud_msc_inquiry_cb(uint8_t lun,uint8_t v[8],uint8_t p[16],uint8_t r[4]){
 (void)lun;memcpy(v,"A8Duo  ",8);memcpy(p,"Library A       ",16);memcpy(r,"0002",4);
}
bool tud_msc_test_unit_ready_cb(uint8_t lun){
 if(ejected||!fatfs_is_mounted()){tud_msc_set_sense(lun,SCSI_SENSE_NOT_READY,0x3a,0);return false;}return true;
}
void tud_msc_capacity_cb(uint8_t lun,uint32_t *count,uint16_t *size){(void)lun;*count=SECTOR_NUM;*size=512;}
bool tud_msc_start_stop_cb(uint8_t lun,uint8_t condition,bool start,bool eject){
 (void)lun;(void)condition;if(eject){if(start){ejected=false;return mount_fatfs_disk();}if(!fatfs_disk_sync_ok())return false;ejected=true;}return true;
}
bool tud_msc_is_writable_cb(uint8_t lun){return tud_msc_test_unit_ready_cb(lun);}
int32_t tud_msc_read10_cb(uint8_t lun,uint32_t lba,uint32_t offset,void *b,uint32_t n){
 if(!tud_msc_test_unit_ready_cb(lun)||offset||n!=512||fatfs_disk_read(b,lba,1))return -1;return 512;
}
int32_t tud_msc_write10_cb(uint8_t lun,uint32_t lba,uint32_t offset,uint8_t *b,uint32_t n){
 if(!tud_msc_test_unit_ready_cb(lun)||offset||n!=512||fatfs_disk_write(b,lba,1))return -1;return 512;
}
int32_t tud_msc_scsi_cb(uint8_t lun,const uint8_t cmd[16],void *b,uint16_t n){
 (void)b;(void)n;if(cmd[0]==0x35)return fatfs_disk_sync_ok()?0:-1;
 tud_msc_set_sense(lun,SCSI_SENSE_ILLEGAL_REQUEST,0x20,0);return -1;
}

void tud_msc_write10_complete_cb(uint8_t lun){(void)lun;fatfs_disk_sync();}
