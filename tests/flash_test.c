#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include "flash_fs.h"
#define SIZE (16u*1024u*1024u)
#define BASE 0x10000000u
static uint8_t *mem;static unsigned erases[4096];
void flash_range_erase(uint32_t a,size_t n){assert(a>=1048576&&a<SIZE&&n<=SIZE-a&&a%4096==0&&n%4096==0);memset(mem+a,255,n);for(size_t i=0;i<n;i+=4096)erases[(a+i)/4096]++;}
void flash_range_program(uint32_t a,const uint8_t *p,size_t n){assert(a>=1048576&&a<SIZE&&n<=SIZE-a&&a%256==0&&n%256==0);for(size_t i=0;i<n;i++){assert((mem[a+i]&p[i])==p[i]);mem[a+i]&=p[i];}}
static void pattern(uint8_t *b,unsigned sector,unsigned round){for(unsigned i=0;i<512;i++)b[i]=(uint8_t)((sector*31u+i*17u+round*73u)^(sector>>8));}
int main(void){mem=mmap((void*)(uintptr_t)BASE,SIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);assert(mem==(void*)(uintptr_t)BASE);memset(mem,255,SIZE);assert(flash_fs_blank());assert(flash_fs_mount()!=0);flash_fs_create();assert(flash_fs_mount()==0);uint8_t b[512],v[512];
 for(unsigned round=0;round<3;round++){
  for(unsigned i=0;i<30464;i++){pattern(b,i,round);flash_fs_write_FAT_sector(i,b);assert(flash_fs_verify_FAT_sector(i,b));}
  flash_fs_sync();assert(flash_fs_mount()==0);
  for(unsigned i=0;i<30464;i++){pattern(b,i,round);flash_fs_read_FAT_sector(i,v);assert(!memcmp(b,v,512));}
 }
 /* Invalid API sector is refused without accessing the chip. */
 flash_fs_write_FAT_sector(65535,b);memset(v,0xff,512);flash_fs_read_FAT_sector(65535,v);for(int i=0;i<512;i++)assert(v[i]==0);
 assert(!flash_fs_blank());for(unsigned i=0;i<1048576;i++)assert(mem[i]==255);
 /* Corrupt physical mapping and duplicate mapping must be rejected at mount. */
 uint8_t save[4];memcpy(save,mem+1048576+8,4);mem[1048576+8]=0xff;mem[1048576+9]=0xff;assert(flash_fs_mount()!=0);memcpy(mem+1048576+8,save,4);
 memcpy(mem+1048576+10,mem+1048576+8,2);assert(flash_fs_mount()!=0);memcpy(mem+1048576+8,save,4);assert(flash_fs_mount()==0);
 unsigned lo=~0u,hi=0;for(unsigned i=271;i<4096;i++){if(erases[i]<lo)lo=erases[i];if(erases[i]>hi)hi=erases[i];}
 printf("PASS: 91,392 sector writes, three complete fill/read/reboot cycles, 16 MiB physical bounds, firmware protected, invalid/duplicate map rejected; data-block erases %u..%u\n",lo,hi);munmap(mem,SIZE);}
