/* Standard decoded CAS records, presented through the existing ATR command
 * transport. Source files are read-only; no conversion files or flash writes.
 * This is original A8Duo code, not code from MeanHamster or a8-pico-sio. */
#include "cas_file.h"
#include <string.h>
#include "../../generated/cas_runtime.h"
extern const unsigned char os_rom[];
static FIL source;
static FIL *view;
static bool opened, active;
static char selected[256];
static uint32_t offsets[CAS_MAX_RECORDS];
static uint8_t lengths[CAS_MAX_RECORDS];
static unsigned records;
static uint32_t image_size;
static unsigned word(const uint8_t *p) { return p[0] | ((unsigned)p[1]<<8); }
static bool exact(void *p, unsigned n) { UINT got; return f_read(&source,p,n,&got)==FR_OK && got==n; }
static uint8_t checksum(const uint8_t *p,unsigned n) {
    unsigned sum=0; while(n--){sum+=*p++;sum=(sum&255)+(sum>>8);} return (uint8_t)sum;
}
void cas_reset(void) {
    active=false;view=NULL;records=0;image_size=0;selected[0]=0;
    if(opened){opened=false;f_close(&source);}
}
bool cas_active(void){return active;}
bool cas_matches(const char *p){return active&&!strcmp(p,selected);}
bool cas_is_file(FIL *f){return view && f==view;}
void cas_forget(FIL *f){if(f==view)view=NULL;}
static bool fail(char *error,const char *text){strcpy(error,text);cas_reset();return false;}
bool cas_prepare(const char *path,char error[40]) {
    cas_reset();
    if(strlen(path)>=sizeof selected)return fail(error,"CAS path too long");
    if(f_open(&source,path,FA_READ)!=FR_OK)return fail(error,"Can't open CAS");
    opened=true;
    uint8_t h[8],r[132],boot[6]={0};unsigned boot_count=0;bool first=true;
    while(f_tell(&source)<f_size(&source)){
        if(!exact(h,8))return fail(error,"Truncated CAS header");
        unsigned n=word(h+4),aux=word(h+6);
        if(n>f_size(&source)-f_tell(&source))return fail(error,"Truncated CAS chunk");
        if(first && memcmp(h,"FUJI",4))return fail(error,"Missing CAS FUJI header");
        first=false;
        if(!memcmp(h,"FUJI",4)){
            if(f_lseek(&source,f_tell(&source)+n)!=FR_OK)return fail(error,"CAS read error");
        }else if(!memcmp(h,"baud",4)){
            if(n||!aux)return fail(error,"Invalid CAS baud chunk");
        }else if(!memcmp(h,"data",4)){
            if(n<5||n>132)return fail(error,"Nonstandard CAS record");
            if(records==CAS_MAX_RECORDS)return fail(error,"CAS exceeds 4096 records");
            offsets[records]=f_tell(&source);lengths[records]=(uint8_t)n;
            if(!exact(r,n))return fail(error,"CAS read error");
            if(r[0]!=0x55||r[1]!=0x55)return fail(error,"Nonstandard CAS sync");
            if(checksum(r,n-1)!=r[n-1])return fail(error,"CAS checksum error");
            unsigned payload;
            if(r[2]==0xfc){if(n!=132)return fail(error,"Short full CAS record");payload=128;}
            else if(r[2]==0xfa){payload=r[n-2];if(payload>127||payload>n-5)return fail(error,"Invalid partial CAS record");}
            else if(r[2]==0xfe)payload=0;
            else return fail(error,"Nonstandard CAS record type");
            if(!records){
                if(payload<6)return fail(error,"CAS has no boot header");
                memcpy(boot,r+3,6);boot_count=boot[1]?boot[1]:256;
                unsigned load=word(boot+2),end=load+128*boot_count;
                if(load<0x0480||end>0xc000)return fail(error,"CAS boot address unsupported");
            }
            if(records<boot_count && r[2]==0xfe)return fail(error,"CAS ends before boot is loaded");
            records++;
        }else if(!memcmp(h,"fsk ",4)||!memcmp(h,"pwm",3)){
            return fail(error,"Turbo/FSK CAS not supported");
        }else return fail(error,"Unsupported CAS chunk");
    }
    if(first||!records||records<boot_count)return fail(error,"Incomplete boot CAS");
    strcpy(selected,path);image_size=16+128*(1+2*records);active=true;
    return true;
}
FRESULT cas_open(FIL *f){
    if(!active)return FR_NOT_READY;
    if(view&&view!=f)return FR_TOO_MANY_OPEN_FILES;
    memset(f,0,sizeof *f);f->obj.objsize=image_size;view=f;return FR_OK;
}
FRESULT cas_seek(FIL *f,FSIZE_t off){
    if(!cas_is_file(f))return FR_INVALID_OBJECT;
    if(off>image_size)return FR_INVALID_PARAMETER;
    f->fptr=off;return FR_OK;
}
FRESULT cas_close(FIL *f){if(!cas_is_file(f))return FR_INVALID_OBJECT;view=NULL;return FR_OK;}
static FRESULT record(unsigned i,uint8_t out[132]){
    uint8_t raw[132];unsigned n=lengths[i];
    if(f_lseek(&source,offsets[i])!=FR_OK||!exact(raw,n))return FR_DISK_ERR;
    memset(out,0,132);memcpy(out,raw,n-1);
    if(raw[2]==0xfa){unsigned used=raw[n-2];memset(out+3+used,0,128-used);out[130]=(uint8_t)used;}
    out[131]=checksum(out,131);return FR_OK;
}
FRESULT cas_read(FIL *f,void *buf,UINT n,UINT *got){
    *got=0;if(!cas_is_file(f)||f->fptr>image_size)return FR_INVALID_OBJECT;
    if(n>image_size-f->fptr)n=image_size-f->fptr;
    uint8_t *dst=buf;
    while(n){
        uint8_t block[132];unsigned avail,offset;
        if(f->fptr<16){
            memset(block,0,16);unsigned paras=(image_size-16)/16;
            block[0]=0x96;block[1]=2;block[2]=paras;block[3]=paras>>8;block[4]=128;block[6]=paras>>16;
            offset=f->fptr;avail=16-offset;
        }else{
            unsigned pos=f->fptr-16,sector=pos/128+1;offset=pos%128;avail=128-offset;
            if(sector==1){memset(block,0,128);memcpy(block,cas_boot_sector,sizeof cas_boot_sector);}
            else{
                unsigned i=(sector-2)/2;FRESULT rc=record(i,block);if(rc!=FR_OK)return rc;
                if((sector-2)&1){memmove(block,block+128,4);memset(block+4,0,124);}
            }
        }
        unsigned take=n<avail?n:avail;memcpy(dst,block+offset,take);
        dst+=take;n-=take;f->fptr+=take;*got+=take;
    }
    return FR_OK;
}
void cas_patch_os(uint8_t *dst){
    /* Only CAS uses this known, bundled OS. Custom UNO_OS.ROM remains untouched
     * for normal ATR use. Reserved filler range is asserted by the generator. */
    memcpy(dst,os_rom,16384);
    memcpy(dst+CAS_RUNTIME_ADDRESS-0xc000,cas_runtime,sizeof cas_runtime);
}
