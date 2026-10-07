/* Virtual top directory without editing atari_cart.c or Atari boot ROM.
   GNU ld --wrap adapts only FatFs calls. Real operations still use upstream FatFs. */
#include "ff.h"
#include "fatfs_disk.h"
#include <string.h>
#include "tusb.h"
#include "game_virtual.h"
FRESULT __real_f_mount(FATFS *,const TCHAR *,BYTE);
FRESULT __real_f_open(FIL *,const TCHAR *,BYTE);
FRESULT __real_f_opendir(DIR *,const TCHAR *);
FRESULT __real_f_readdir(DIR *,FILINFO *);
FRESULT __real_f_closedir(DIR *);
static FATFS volumes[2];static bool registered;
/* A search may hold one virtual root plus real subdirectories recursively. */
static DIR *virtual_root;static unsigned root_index;
static bool ui(void){return !tud_mounted();}
static void ensure(void){if(!registered){__real_f_mount(&volumes[0],"0:",0);__real_f_mount(&volumes[1],"1:",0);registered=true;}}
static bool translate(const char *src,char dst[260]){
 if(*src=='/')src++;
 if(!strncmp(src,"LIBRARYA",8)&&(src[8]=='/'||!src[8])){dst[0]='0';src+=8;}
 else if(!strncmp(src,"LIBRARYB",8)&&(src[8]=='/'||!src[8])){dst[0]='1';src+=8;}
 else{dst[0]='0';} /* UNO_OS.ROM retains its original lookup in Library A. */
 dst[1]=':';dst[2]='/';if(*src=='/')src++;size_t n=strlen(src);if(n>256)return false;memcpy(dst+3,src,n+1);return true;
}
FRESULT __wrap_f_mount(FATFS *fs,const TCHAR *path,BYTE opt){
 if(!ui()||(*path && path[1]==':')){registered=false;return __real_f_mount(fs,path,opt);}
 (void)fs;(void)opt;ensure();return FR_OK; /* Persistent owners avoid dangling stack FATFS objects. */
}
FRESULT __wrap_f_opendir(DIR *d,const TCHAR *path){
 if(!ui())return __real_f_opendir(d,path);ensure();
 if(!*path||!strcmp(path,"/")){if(virtual_root&&virtual_root!=d)return FR_TOO_MANY_OPEN_FILES;memset(d,0,sizeof(*d));virtual_root=d;root_index=0;return FR_OK;}
 char translated[260];if(!translate(path,translated))return FR_INVALID_NAME;return __real_f_opendir(d,translated);
}
FRESULT __wrap_f_readdir(DIR *d,FILINFO *f){
 if(d!=virtual_root)return __real_f_readdir(d,f);
 if(!f){root_index=0;return FR_OK;}memset(f,0,sizeof(*f));
 while(root_index<3){unsigned n=root_index++;if(n==2){strcpy(f->fname,"Worlds Hardest Game.ATR");strcpy(f->altname,"HARDGAME.ATR");f->fsize=16+1024*128;break;}if(n==0&&!fatfs_is_mounted())continue;if(n==1&&!duo_remote_ready())continue;
  strcpy(f->fname,n?"Library B":"Library A");strcpy(f->altname,n?"LIBRARYB":"LIBRARYA");f->fattrib=AM_DIR;break;}
 return FR_OK;
}
FRESULT __wrap_f_closedir(DIR *d){if(d==virtual_root){virtual_root=0;return FR_OK;}return __real_f_closedir(d);}
FRESULT __wrap_f_open(FIL *f,const TCHAR *path,BYTE mode){game_forget(f);if(!ui())return __real_f_open(f,path,mode);if(game_path(path))return game_open(f);ensure();char translated[260];if(!translate(path,translated))return FR_INVALID_NAME;return __real_f_open(f,translated,mode);}
