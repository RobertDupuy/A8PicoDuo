#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "block_transport.h"
bool flood;uint16_t input[2048];unsigned used,cursor;uart_hw_t reg;
static void wire(uint8_t*f,unsigned n){used=n;cursor=0;for(unsigned i=0;i<n;i++)input[i]=f[i];}
int main(void){
 assert(nx_crc32((const uint8_t*)"123456789",9)==0xcbf43926);
 uint8_t f[536],r[536];nx_header(f,NX_WRITE,0,0xfedcba98,516);nx_put32(f+16,0x03ffffff);
 for(unsigned i=20;i<532;i++)f[i]=(uint8_t)i;
 nx_put32(f+532,nx_crc32(f,532));wire(f,536);assert(nx_receive(0,r,100000));assert(memcmp(f,r,536)==0);
 assert(nx_get32(r+8)==0xfedcba98&&nx_get32(r+16)==0x03ffffff);
 wire(f,536);input[100]^=1;assert(!nx_receive(0,r,100000));
 wire(f,536);input[50]|=0x100;assert(!nx_receive(0,r,100000));
 wire(f,20);assert(!nx_receive(0,r,1000));
 nx_put16(f+12,517);wire(f,536);memset(r,0xa5,sizeof r);assert(!nx_receive(0,r,100000));for(unsigned i=16;i<536;i++)assert(r[i]==0xa5);
 nx_put16(f+12,516);f[4]=2;wire(f,536);assert(!nx_receive(0,r,100000));
 f[4]=1;nx_put32(f+532,nx_crc32(f,532));wire(f,536);memmove(input+5,input,536*sizeof input[0]);used+=5;for(int i=0;i<5;i++)input[i]=i+19;assert(nx_receive(0,r,100000));
 used=cursor=0;flood=true;assert(!nx_receive(0,r,100000));flood=false;
 puts("PASS: continuous-noise deadline, CRC vector, full 516-byte frame, 32-bit sequence/LBA, corrupt CRC, UART error, timeout, oversize bounds, version, noise resynchronization.");
}
