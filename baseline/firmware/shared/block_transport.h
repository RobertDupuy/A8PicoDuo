#pragma once
#include "block_protocol.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
/* Polling only. No IRQ, DMA, alarm or multicore work is installed here. */
static bool nx_read_bytes(uart_inst_t *u,uint8_t *p,size_t n,uint64_t end) {
 while(n){if(time_us_64()>=end)return false;if(uart_is_readable(u)){uint32_t v=uart_get_hw(u)->dr;if(v&0xf00)return false;*p++=(uint8_t)v;n--;}
 else if(time_us_64()>=end)return false;}return true;
}
static bool nx_receive(uart_inst_t *u,uint8_t *f,uint64_t end) {
 uint32_t magic=0; uint8_t b;
 do{if(!nx_read_bytes(u,&b,1,end))return false;magic=(magic>>8)|((uint32_t)b<<24);}while(magic!=NX_MAGIC);
 nx_put32(f,NX_MAGIC);
 if(!nx_read_bytes(u,f+4,NX_HEADER-4,end))return false;
 uint16_t n=nx_get16(f+12);
 if(f[4]!=NX_VERSION||nx_get16(f+14)!=0||n>NX_MAX_PAYLOAD)return false;
 if(!nx_read_bytes(u,f+NX_HEADER,n+4,end))return false;
 return nx_get32(f+NX_HEADER+n)==nx_crc32(f,NX_HEADER+n);
}
static bool nx_send(uart_inst_t *u,uint8_t *f,uint64_t end) {
 size_t n=NX_HEADER+nx_get16(f+12); nx_put32(f+n,nx_crc32(f,n));n+=4;
 for(size_t i=0;i<n;i++){while(!uart_is_writable(u))if(time_us_64()>=end)return false;uart_get_hw(u)->dr=f[i];}
 while(uart_get_hw(u)->fr & UART_UARTFR_BUSY_BITS)if(time_us_64()>=end)return false;
 return true;
}
