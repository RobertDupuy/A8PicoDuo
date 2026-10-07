/* Embedded ATR and RAM-only command sectors. Original Atari engine unchanged. */
#include <string.h>
#include <stdbool.h>
#include "ff.h"
#include "game_virtual.h"
#include "cas_file.h"
#include "block_client.h"
#include "../../game/generated/atari_image.h"
#include "../../game/generated/display.h"
#define IMAGE_SIZE (16u+1024u*128u)
#define SECTOR_OFFSET(n) (16u+((n)-1u)*128u)
static FIL *virtual_file;
static uint8_t snapshot[2048];
FRESULT __real_f_read(FIL*,void*,UINT,UINT*);
FRESULT __real_f_write(FIL*,const void*,UINT,UINT*);
FRESULT __real_f_lseek(FIL*,FSIZE_t);
FRESULT __real_f_sync(FIL*);
FRESULT __real_f_close(FIL*);
bool game_path(const char *p){if(*p=='/')p++;return !strcmp(p,"HARDGAME.ATR");}
void game_forget(FIL *f){if(f==virtual_file)virtual_file=0;}
FRESULT game_open(FIL *f){
    memset(f,0,sizeof(*f));f->obj.objsize=IMAGE_SIZE;virtual_file=f;
    memset(snapshot,0,sizeof snapshot);snapshot[0]='H';snapshot[1]='G';snapshot[2]=HG_PROTOCOL_VERSION;snapshot[3]=1;
    return FR_OK;
}
static void failure(void){memset(snapshot,0,sizeof snapshot);snapshot[0]='H';snapshot[1]='G';snapshot[2]=HG_PROTOCOL_VERSION;snapshot[3]=1;}
FRESULT __wrap_f_read(FIL *f,void *buf,UINT n,UINT *got){
    if(cas_is_file(f))return cas_read(f,buf,n,got);
    if(f!=virtual_file)return __real_f_read(f,buf,n,got);
    *got=0;if(f->fptr>IMAGE_SIZE)return FR_INT_ERR;
    if(n>IMAGE_SIZE-f->fptr)n=IMAGE_SIZE-f->fptr;
    if(n==128&&f->fptr>=SECTOR_OFFSET(1001)&&f->fptr<SECTOR_OFFSET(1017)&&!((f->fptr-SECTOR_OFFSET(1001))%128)){
        memcpy(buf,snapshot+f->fptr-SECTOR_OFFSET(1001),128);
    }else{
        static const uint8_t header[16]={0x96,0x02,0x00,0x20,0x80,0,0,0,0,0,0,0,0,0,0,0};
        uint8_t *d=buf;
        for(UINT i=0;i<n;i++){
            FSIZE_t at=f->fptr+i;
            d[i]=at<16?header[at]:(at-16<sizeof(hg_atari_image)?hg_atari_image[at-16]:0);
        }
    }
    f->fptr+=n;*got=n;return FR_OK;
}
FRESULT __wrap_f_write(FIL *f,const void *buf,UINT n,UINT *done){
    if(cas_is_file(f)){*done=0;return FR_WRITE_PROTECTED;}
    if(f!=virtual_file)return __real_f_write(f,buf,n,done);
    *done=0;const uint8_t *b=buf;
    if(f->fptr!=SECTOR_OFFSET(1000)||n!=128)return FR_DENIED;
    if(b[0]!='H'||b[1]!='G'||!nx_game_command(b+2,snapshot))failure();
    f->fptr+=128;*done=128;return FR_OK;
}
FRESULT __wrap_f_lseek(FIL *f,FSIZE_t off){if(cas_is_file(f))return cas_seek(f,off);if(f!=virtual_file)return __real_f_lseek(f,off);if(off>IMAGE_SIZE)return FR_INVALID_PARAMETER;f->fptr=off;return FR_OK;}
FRESULT __wrap_f_sync(FIL *f){return (f==virtual_file||cas_is_file(f))?FR_OK:__real_f_sync(f);}
FRESULT __wrap_f_close(FIL *f){if(cas_is_file(f))return cas_close(f);if(f!=virtual_file)return __real_f_close(f);virtual_file=0;return FR_OK;}
