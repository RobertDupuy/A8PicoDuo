/* Test transport connects real U1 virtual-file code to the real U2 game core. */
#include <string.h>
#include "ff.h"
#include "game_virtual.h"
#include "game_engine.h"
static FIL disk;
static unsigned flash_calls;
static int offline;
FRESULT __wrap_f_read(FIL*,void*,UINT,UINT*);
FRESULT __wrap_f_write(FIL*,const void*,UINT,UINT*);
FRESULT __wrap_f_lseek(FIL*,FSIZE_t);
FRESULT __wrap_f_sync(FIL*);
FRESULT __wrap_f_close(FIL*);
bool nx_game_command(const uint8_t*in,uint8_t*b){if(offline||!hg_command(in,b))return false;for(int p=1;p<4;p++)if(!hg_page(p,b+p*512))return false;return true;}
FRESULT __real_f_read(FIL*f,void*b,UINT n,UINT*r){(void)f;(void)b;(void)n;*r=0;flash_calls++;return FR_NOT_READY;}
FRESULT __real_f_write(FIL*f,const void*b,UINT n,UINT*r){(void)f;(void)b;(void)n;*r=0;flash_calls++;return FR_NOT_READY;}
FRESULT __real_f_lseek(FIL*f,FSIZE_t n){(void)f;(void)n;flash_calls++;return FR_NOT_READY;}
FRESULT __real_f_sync(FIL*f){(void)f;flash_calls++;return FR_NOT_READY;}
FRESULT __real_f_close(FIL*f){(void)f;flash_calls++;return FR_NOT_READY;}
void bridge_open(void){game_open(&disk);offline=0;flash_calls=0;}
void bridge_offline(int value){offline=value;}
unsigned bridge_flash_calls(void){return flash_calls;}
int bridge_read(unsigned sector,uint8_t *buf){UINT n;return __wrap_f_lseek(&disk,16+(sector-1)*128)==FR_OK&&__wrap_f_read(&disk,buf,128,&n)==FR_OK&&n==128?0:2;}
int bridge_write(unsigned sector,const uint8_t *buf){UINT n;return __wrap_f_lseek(&disk,16+(sector-1)*128)==FR_OK&&__wrap_f_write(&disk,buf,128,&n)==FR_OK&&n==128&&__wrap_f_sync(&disk)==FR_OK?0:2;}
int bridge_read_at(unsigned off,unsigned len,uint8_t *buf){UINT n;return __wrap_f_lseek(&disk,off)==FR_OK&&__wrap_f_read(&disk,buf,len,&n)==FR_OK?(int)n:-1;}
int bridge_passthrough(void){FIL other;UINT n;uint8_t b[2];flash_calls=0;__wrap_f_read(&other,b,2,&n);__wrap_f_write(&other,b,2,&n);__wrap_f_sync(&other);__wrap_f_lseek(&other,0);__wrap_f_close(&other);return flash_calls==5;}
