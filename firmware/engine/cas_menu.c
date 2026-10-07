/* Menu-only dispatch kept in flash to leave the pinned SRAM bus code layout
 * undisturbed. Ordinary ROM/CAR/XEX/ATR handling is inherited verbatim. */
#include "cas_file.h"
#include <string.h>
#include <strings.h>
extern unsigned char cart_ram[],cart_d5xx[];
extern const unsigned char os_rom[];
extern char errorBuf[];
extern int load_file(char *filename);
#define CART_TYPE_NONE 0
#define CART_TYPE_ATR 254
#define CART_TYPE_XEX 255
void cas_menu_select(char *path,char *filename,int *cart_type){
    cas_reset();
    char *dot=strrchr(filename,'.');const char *ext=dot?dot+1:"";
    if(!strcasecmp(ext,"CAS")){
        if(cas_prepare(path,errorBuf)){cart_d5xx[1]=3;*cart_type=CART_TYPE_ATR;}
        else{*cart_type=CART_TYPE_NONE;cart_d5xx[1]=4;strcpy((char*)cart_d5xx+2,errorBuf);}
    }else if(!strcasecmp(ext,"ATR")){cart_d5xx[1]=3;*cart_type=CART_TYPE_ATR;}
    else{
        *cart_type=load_file(path);
        if(*cart_type)cart_d5xx[1]=*cart_type!=CART_TYPE_XEX?1:2;
        else{cart_d5xx[1]=4;strcpy((char*)cart_d5xx+2,errorBuf);}
    }
}
void cas_menu_load_os(void){
    if(cas_active())cas_patch_os(cart_ram);
    else if(!load_file("UNO_OS.ROM"))memcpy(cart_ram,os_rom,16384);
    cart_d5xx[1]=0;
}
